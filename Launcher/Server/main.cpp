#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <string>

#include "ServerConfig.h"
#include "ServerMain.h"

static volatile bool g_running = true;

static BOOL WINAPI consoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        printf("[Server] Shutting down...\n");
        g_running = false;
        return TRUE;
    }
    return FALSE;
}

static DWORD WINAPI stdinThread(LPVOID) {
    char line[256];
    while (g_running) {
        if (fgets(line, sizeof(line), stdin) == nullptr)
            break;
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (len == 0) continue;

        if (strcmp(line, "stop") == 0 || strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            printf("[Server] Shutting down...\n");
            g_running = false;
        } else if (strcmp(line, "help") == 0 || strcmp(line, "?") == 0) {
            printf("Commands: stop, help, status\n");
        } else if (strcmp(line, "status") == 0) {
            printf("[Server] Running: %s, Players: %d\n",
                   DedicatedServer::isRunning() ? "yes" : "no",
                   DedicatedServer::getPlayerCount());
        } else {
            printf("[Server] Unknown command: %s (type 'help')\n", line);
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("========================================\n");
    printf("  Ourcraft Dedicated Server\n");
    printf("  (Legacy Console Edition)\n");
    printf("========================================\n\n");

    SetConsoleCtrlHandler(consoleHandler, TRUE);

    const char* configPath = "server.properties";
    printf("[Server] Loading config from %s...\n", configPath);
    ServerConfig config = ServerConfig::load(configPath);

    printf("[Server] Port:         %u\n", config.bedrockPort);
    printf("[Server] Max Players:  %d\n", config.maxPlayers);
    printf("[Server] View Dist:    %d\n", config.viewDistance);
    printf("[Server] Online Mode:  %s\n", config.onlineMode ? "true" : "false");
    printf("\n");

    ServerInitParams params;
    params.bedrockPort = config.bedrockPort;
    params.javaPort    = config.javaPort;
    params.maxPlayers  = config.maxPlayers;
    params.onlineMode  = config.onlineMode;
    params.motd        = config.motd.c_str();
    params.worldName   = config.worldName.c_str();
    params.gamemode    = config.gamemode.c_str();
    params.difficulty  = config.difficulty.c_str();
    params.seed        = config.seed;
    params.findSeed    = config.findSeed;

    if (!DedicatedServer::init(params)) {
        printf("[Server] ERROR: Failed to start on port %u\n", config.bedrockPort);
        return 1;
    }

    printf("[Server] Listening on port %u (max %d players)\n", config.bedrockPort, config.maxPlayers);
    printf("[Server] Server ready. Type 'help' for commands.\n\n");

    HANDLE hStdin = CreateThread(nullptr, 0, stdinThread, nullptr, 0, nullptr);

    const DWORD tickMs = 50;
    uint64_t tickCount = 0;

    while (g_running && DedicatedServer::isRunning()) {
        DWORD tickStart = GetTickCount();

        DedicatedServer::tick();
        tickCount++;

        DWORD elapsed = GetTickCount() - tickStart;
        if (elapsed < tickMs) {
            Sleep(tickMs - elapsed);
        }
    }

    printf("[Server] Stopping...\n");
    DedicatedServer::shutdown();
    printf("[Server] Server stopped after %llu ticks.\n", tickCount);

    if (hStdin) {
        TerminateThread(hStdin, 0);
        CloseHandle(hStdin);
    }

    return 0;
}

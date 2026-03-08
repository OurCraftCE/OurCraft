#pragma once

#include "../../../World/x64headers/extraX64.h"

// Maps Discord user IDs (int64_t / snowflake) to the game's PlayerUID (XUID / ULONGLONG).
// Uses the high bit (bit 63) to distinguish Discord-derived XUIDs from real Xbox XUIDs.
// Real Xbox XUIDs never have bit 63 set, so this is a safe sentinel.

class DiscordXUIDMapper
{
public:
    // Generate a stable XUID from a Discord user ID.
    // Sets bit 63 to mark it as a Discord-derived XUID.
    static PlayerUID DiscordIdToXuid(int64_t discordId)
    {
        return (PlayerUID)(0x8000000000000000ULL | (ULONGLONG)discordId);
    }

    // Extract the Discord user ID from a Discord-derived XUID.
    static int64_t XuidToDiscordId(PlayerUID xuid)
    {
        return (int64_t)(xuid & 0x7FFFFFFFFFFFFFFFULL);
    }

    // Check whether a given XUID was generated from a Discord user ID.
    static bool IsDiscordXuid(PlayerUID xuid)
    {
        return (xuid & 0x8000000000000000ULL) != 0;
    }

    // Populate a gamertag buffer (wchar_t[32]) from a Discord username (UTF-8).
    // Truncates to 31 characters to match IQNetPlayer::m_gamertag size.
    static void UsernameToGamertag(const char* discordUsername, wchar_t* gamertagOut, int gamertagMaxLen = 32)
    {
        if (!discordUsername || !gamertagOut || gamertagMaxLen <= 0) return;

        // Convert UTF-8 Discord username to wide string
        int written = MultiByteToWideChar(CP_UTF8, 0, discordUsername, -1,
                                          gamertagOut, gamertagMaxLen);
        if (written == 0)
        {
            // Fallback: just copy ASCII
            int i = 0;
            for (; discordUsername[i] && i < gamertagMaxLen - 1; ++i)
            {
                gamertagOut[i] = (wchar_t)discordUsername[i];
            }
            gamertagOut[i] = L'\0';
        }
    }
};

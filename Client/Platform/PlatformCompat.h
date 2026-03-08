#pragma once
#if defined(__EMSCRIPTEN__) || defined(__linux__) || defined(__APPLE__)

// ---------------------------------------------------------------------------
// PlatformCompat.h — Win32 type & function stubs for non-Windows builds.
// Covers: WASM (Emscripten), Linux, macOS.
// ---------------------------------------------------------------------------

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <wchar.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

// ---------------------------------------------------------------------------
// Basic Win32 types
// ---------------------------------------------------------------------------
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned int        DWORD;
typedef unsigned long long  ULONGLONG;
typedef long long           LONGLONG;
typedef long                LONG;
typedef unsigned int        UINT;
typedef int                 INT;
typedef float               FLOAT;

typedef void*               HANDLE;
typedef void*               HWND;
typedef void*               HINSTANCE;
typedef void*               HMODULE;
typedef void*               HDC;
typedef void*               LPVOID;
typedef const void*         LPCVOID;
typedef long                HRESULT;

typedef wchar_t             WCHAR;
typedef wchar_t*            LPWSTR;
typedef const wchar_t*      LPCWSTR;
typedef const wchar_t*      PCWSTR;
typedef char*               LPSTR;
typedef const char*         LPCSTR;
typedef unsigned char*      PBYTE;
typedef DWORD*              LPDWORD;

#define TRUE  1
#define FALSE 0
#define S_OK  0
#define FAILED(hr) ((hr) < 0)
#define SUCCEEDED(hr) ((hr) >= 0)
#define HRESULT_SUCCEEDED(hr) ((hr) >= 0)

#define WINAPI
#define APIENTRY
#define CALLBACK
#define UNREFERENCED_PARAMETER(p) ((void)(p))

#define MAX_PATH 260

using std::wstring;
using std::string;

// ---------------------------------------------------------------------------
// Critical section (POSIX mutex)
// ---------------------------------------------------------------------------
typedef pthread_mutex_t CRITICAL_SECTION;

inline void InitializeCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_init(cs, nullptr);
}
inline void InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION* cs, DWORD) {
    pthread_mutex_init(cs, nullptr);
}
inline void EnterCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_lock(cs);
}
inline void LeaveCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_unlock(cs);
}
inline void DeleteCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_destroy(cs);
}

// ---------------------------------------------------------------------------
// Sleep / timing
// ---------------------------------------------------------------------------
inline void Sleep(DWORD ms) { usleep(ms * 1000); }

// ---------------------------------------------------------------------------
// String helpers
// ---------------------------------------------------------------------------
inline int MultiByteToWideChar(UINT, DWORD, const char* src, int, wchar_t* dst, int dstLen) {
    return (int)mbstowcs(dst, src, dstLen);
}
inline int WideCharToMultiByte(UINT, DWORD, const wchar_t* src, int, char* dst, int dstLen, const char*, BOOL*) {
    return (int)wcstombs(dst, src, dstLen);
}

#ifndef sprintf_s
#define sprintf_s(buf, size, ...) snprintf(buf, size, __VA_ARGS__)
#endif
#ifndef strcpy_s
#define strcpy_s(dst, size, src) strncpy(dst, src, size)
#endif
#ifndef strncpy_s
#define strncpy_s(dst, size, src, count) strncpy(dst, src, count)
#endif
#ifndef wcscpy_s
#define wcscpy_s(dst, size, src) wcsncpy(dst, src, size)
#endif
#ifndef swprintf_s
#define swprintf_s(buf, size, ...) swprintf(buf, size, __VA_ARGS__)
#endif
#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif

// OutputDebugString → stderr
#define OutputDebugStringA(s) fprintf(stderr, "%s", s)
#define OutputDebugStringW(s) fwprintf(stderr, L"%ls", s)
#define OutputDebugString(s)  OutputDebugStringW(s)

// PIX → no-op
#define PIXBeginNamedEvent(c, n) ((void)0)
#define PIXEndNamedEvent()       ((void)0)

// MemSect → no-op
#define MemSect(x) ((void)0)

// _T macro
#define _T(x) L##x

// ---------------------------------------------------------------------------
// Filesystem helpers
// ---------------------------------------------------------------------------
inline BOOL CreateDirectoryW(LPCWSTR path, void*) {
    char buf[MAX_PATH];
    wcstombs(buf, path, sizeof(buf));
    return (mkdir(buf, 0755) == 0 || errno == EEXIST) ? TRUE : FALSE;
}

inline DWORD GetModuleFileNameW(HMODULE, LPWSTR buf, DWORD size) {
    if (buf && size > 0) buf[0] = 0;
#if defined(__linux__)
    char tmp[MAX_PATH];
    ssize_t n = readlink("/proc/self/exe", tmp, sizeof(tmp) - 1);
    if (n > 0) { tmp[n] = 0; mbstowcs(buf, tmp, size); return (DWORD)n; }
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    char tmp[MAX_PATH]; uint32_t sz = sizeof(tmp);
    if (_NSGetExecutablePath(tmp, &sz) == 0) { mbstowcs(buf, tmp, size); return (DWORD)sz; }
#endif
    return 0;
}

inline BOOL GetUserNameA(LPSTR buf, LPDWORD size) {
    const char* user = getenv("USER");
    if (!user) user = "Player";
    if (buf && size) { strncpy(buf, user, *size); *size = (DWORD)strlen(user) + 1; }
    return TRUE;
}

#endif // EMSCRIPTEN || linux || APPLE

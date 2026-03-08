#include <windows.h>

namespace std {
    const char* _Winerror_map(int errcode)
    {
        static __declspec(thread) char buf[256];
        DWORD len = FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, (DWORD)errcode, 0, buf, sizeof(buf), NULL);
        if (len == 0)
            buf[0] = '\0';
        return buf;
    }
}

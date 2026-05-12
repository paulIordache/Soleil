#include <cstdarg>
#include <cstdio>
#include "../include/windows-macros.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/stat.h>
#endif

void VkEngineError(const char *file, int line, const char *format, ...) {
    char msg[1000];
    std::va_list args;
    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);

#ifdef _WIN32
    char msg2[1200];
    snprintf(msg2, sizeof(msg2), "%s:%d: %s", file, line, msg);
    MessageBoxA(nullptr, msg2, "Vulkan Engine Error", MB_ICONERROR);
#else
    fprintf(stderr, "%s:%d - %s\n", pFileName, line, msg);
#endif
}

void VkEngineFileError(const char *pFileName, int line, const char *pFileError) {
#ifdef _WIN32
    char msg[1000];
    snprintf(msg, sizeof(msg), "%s:%d: unable to open file `%s`", pFileName, line, pFileError);
    MessageBoxA(nullptr, msg, "Vulkan Engine File Error", MB_ICONERROR);
#else
    fprintf(stderr, "%s:%d: unable to open file `%s`\n", pFileName, line, pFileError);
#endif
}

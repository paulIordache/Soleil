#ifndef MACROS_H
#define MACROS_H

#ifndef _WIN64

#include <unistd.h>

#endif

#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <cassert>



void VkEngineError(const char* file, int line, const char* msg, ...);
void VkEngineFileError(const char* file, unsigned int line, const char* pFileError);

#define VK_ENGINE_ERROR0(msg) VkEngineError(__FILE__, __LINE__, msg);
#define VK_ENGINE_ERROR(msg, ...) \
    VkEngineError(__FILE__, __LINE__, msg, __VA_ARGS__);
#define VK_ENGINE_FILE_ERROR(msg) VkEngineFileError(__FILE__, __LINE__, FileError);

#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define ZERO_MEM_VAR(var) memset(&var, 0, sizeof(var))


#endif //MACROS_H

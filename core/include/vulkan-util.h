
#ifndef IP_VULKAN_UTIL_H
#define IP_VULKAN_UTIL_H

#include <cstdio>
#include <cstdlib>

#include "types.h"

#pragma once

#ifndef _WIN64
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <string.h>
#include <assert.h>
#include <time.h>
#ifndef OGLDEV_VULKAN
// #include <GL/glew.h>
#endif
// #include <GL/gl.h>
#include <vulkan/vulkan.h>


#include "types.h"


using namespace std;

bool ReadFile(const char *fileName, std::string &outFile);

char *ReadBinaryFile(const char *pFileName, int &size);

void WriteBinaryFile(const char *pFilename, const void *pData, int size);

void OgldevError(const char *pFileName, u32 line, const char *msg, ...);

void OgldevFileError(const char *pFileName, u32 line, const char *pFileError);

#define OGLDEV_ERROR0(msg) OgldevError(__FILE__, __LINE__, msg)
#define OGLDEV_ERROR(msg, ...) OgldevError(__FILE__, __LINE__, msg, __VA_ARGS__)
#define OGLDEV_FILE_ERROR(FileError) OgldevFileError(__FILE__, __LINE__, FileError);

#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define ZERO_MEM_VAR(var) memset(&var, 0, sizeof(var))
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define ARRAY_SIZE_IN_BYTES(a) (sizeof(a[0]) * a.size())

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifdef _WIN64
#define SNPRINTF _snprintf_s
#define VSNPRINTF vsnprintf_s
#define RANDOM rand
#define SRANDOM srand((unsigned)time(NULL))
#pragma warning (disable: 4566)
#else
#define SNPRINTF snprintf
#define VSNPRINTF vsnprintf
#define RANDOM random
#define SRANDOM srandom(getpid())
#endif

#define SAFE_DELETE(p) if (p) { delete p; p = NULL; }



#define ASSIMP_LOAD_FLAGS (aiProcess_JoinIdenticalVertices |    \
                           aiProcess_Triangulate |              \
                           aiProcess_GenSmoothNormals |         \
                           aiProcess_LimitBoneWeights |         \
                           aiProcess_SplitLargeMeshes |         \
                           aiProcess_ImproveCacheLocality |     \
                           aiProcess_RemoveRedundantMaterials | \
                           aiProcess_FindDegenerates |          \
                           aiProcess_FindInvalidData |          \
                           aiProcess_GenUVCoords |              \
                           aiProcess_CalcTangentSpace)


string GetDirFromFilename(const string &Filename);


#define CLAMP(Val, Start, End) std::min(std::max((Val), (Start)), (End));

static size_t AlignUpToMultiple(const size_t Size, size_t const Alignment) {
    size_t const ret = ((Size + Alignment - 1) / Alignment) * Alignment;
    return ret;
}

#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define ZERO_MEM_VAR(var) memset(&var, 0, sizeof(var))
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define ARRAY_SIZE_IN_BYTES(a) (sizeof(a[0]) * a.size())

#define CHECK_VK_RESULT(res, msg) \
    if (res != VK_SUCCESS) { \
        fprintf(stderr, "Error in %s:%d - %s, code %x\n", __FILE__, __LINE__, msg, res); \
        exit(EXIT_FAILURE); \
    }

const char *GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT severity);

const char *GetDebugType(VkDebugUtilsMessageTypeFlagsEXT type);

int GetBytesPerTexFormat(VkFormat Format);

bool HasStencilComponent(VkFormat Format);

VkFormat FindSupportedFormat(VkPhysicalDevice Device, const std::vector<VkFormat> &Candidates,
                             VkImageTiling Tiling, VkFormatFeatureFlags Features);

#endif //IP_VULKAN_UTIL_H

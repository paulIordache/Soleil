//
// Created by Paul on 9/29/2025.
//

#ifndef FILE_UTIL_H
#define FILE_UTIL_H
#ifdef _WIN32
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>

#include "windows-macros.h"

#else
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#endif
#include <fstream>
#include "types.h"

char *ReadBinaryFile(const char *pFilename, int &size);

void WriteBinaryFile(const char *pFilename, const void *pData, int size);

bool ReadFile(const char *pFileName, str &outFile);

#endif //FILE_UTIL_H

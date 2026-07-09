#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

#include "windows-macros.h"


bool ReadFile(const char *pFileName, std::string &outFile) {
    std::ifstream f(pFileName);

    bool ret = false;

    if (f.is_open()) {
        std::string line;
        while (getline(f, line)) {
            outFile.append(line);
            outFile.append("\n");
        }

        f.close();

        ret = true;
    } else {
        VK_ENGINE_ERROR(pFileName);
    }

    return ret;
}

void WriteBinaryFile(const char *pFilename, const void *pData, int size) {
    FILE *f = nullptr;

    errno_t err = fopen_s(&f, pFilename, "wb");

    if (!f) {
        VK_ENGINE_ERROR("Error opening '%s'\n", pFilename);
        exit(0);
    }

    size_t bytes_written = fwrite(pData, 1, size, f);

    if (bytes_written != size) {
        VK_ENGINE_ERROR("Error write file\n");
        exit(0);
    }

    fclose(f);
}
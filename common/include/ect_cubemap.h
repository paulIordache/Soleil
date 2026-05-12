//
// Created by Paul on 10/21/2025.
//

#ifndef ECT_CUBEMAP_H
#define ECT_CUBEMAP_H

#pragma once

#include <vector>

#include "bitmap.h"

int ConvertEquirectangularImageToCubemap(const Bitmap& b, std::vector<Bitmap>& Cubemap);

#endif //ECT_CUBEMAP_H



#ifndef ECT_CUBEMAP_H
#define ECT_CUBEMAP_H

#pragma once

#include <vector>

#include "bitmap.h"

int ConvertEquirectangularImageToCubemap(const Bitmap& b, std::vector<Bitmap>& Cubemap);

#endif //ECT_CUBEMAP_H

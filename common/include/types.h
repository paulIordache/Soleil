#ifndef TYPES_H
#define TYPES_H
#include <string>
#include <vector>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using s16 = short;
using i32 = int;
using i64 = long long;
using f32 = float;

using d64 = double;
using s8 = char;
using b8 = bool;
using str = std::string;

namespace my_types {
    template <typename T>
    using vec = std::vector<T>;
}


struct VulkanMeshEntry {
    size_t VertexBufferOffset = 0;
    size_t VertexBufferRange = 0;
    size_t IndexBufferOffset = 0;
    size_t IndexBufferRange = 0;
} ;

#endif //TYPES_H

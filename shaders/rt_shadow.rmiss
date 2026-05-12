#version 460
#extension GL_EXT_ray_tracing : require

struct Payload {
    int isShadowed;
};

layout(location = 0) rayPayloadInEXT Payload payload;

void main()
{
    payload.isShadowed = 0; // Miss = Lit
}
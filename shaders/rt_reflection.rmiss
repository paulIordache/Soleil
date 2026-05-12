#version 460
#extension GL_EXT_ray_tracing : require

struct ReflectionPayload {
    vec3 color;
    float distance;
    vec3 hitPos;
    vec3 hitNormal;
    vec3 albedo;
    float targetLod;
};
layout(location = 1) rayPayloadInEXT ReflectionPayload payload;
layout(set = 0, binding = 6) uniform samplerCube skyboxTex;

void main() {
    payload.color = textureLod(skyboxTex, gl_WorldRayDirectionEXT, payload.targetLod).rgb;
    payload.distance = -1.0;
    payload.hitPos = vec3(0.0);
    payload.hitNormal = vec3(0.0);
    payload.albedo = vec3(0.0);
}
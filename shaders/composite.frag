// composite.frag
#version 460

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFinalColor;

layout (binding = 0) uniform sampler2D denoisedLightSampler;
layout (binding = 1) uniform sampler2D albedoSampler;
layout (binding = 2) uniform sampler2D denoisedSpecularSampler;
layout (binding = 3) uniform sampler2D metallicSampler;
layout (binding = 4) uniform sampler2D gNormal;
layout (binding = 5) uniform sampler2D gPos;

layout (push_constant) uniform PushConstants {
    vec3 cameraPos;
    vec3 lightDir;
} pc;

vec3 ACESFilm(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    ivec2 pixelCoord = ivec2(gl_FragCoord.xy);

    vec3 denoisedTotalDiffuse = texelFetch(denoisedLightSampler, pixelCoord, 0).rgb;
    vec3 denoisedIndirectSpecular = texelFetch(denoisedSpecularSampler, pixelCoord, 0).rgb;

    vec3 albedo = texelFetch(albedoSampler, pixelCoord, 0).rgb;
    float metallic = texelFetch(metallicSampler, pixelCoord, 0).r;
    vec4 normalData = texelFetch(gNormal, pixelCoord, 0);
    vec3 Normal = normalize(normalData.xyz);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 finalDiffuse = denoisedTotalDiffuse * albedo;
    vec3 finalIndirectSpec = denoisedIndirectSpecular * max(F0, vec3(0.001));

    vec3 color = finalDiffuse + finalIndirectSpec;

    float exposure = 5.0;
    color *= exposure;

    color = ACESFilm(color);

    outFinalColor = vec4(color, 1.0);
}
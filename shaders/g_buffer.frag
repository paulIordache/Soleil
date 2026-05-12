#version 460
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 v_Normal;
layout(location = 5) in vec4 vCurrentClipPos;
layout(location = 6) in vec4 vPrevClipPos;

layout(location = 0) out vec4 out_Pos;
layout(location = 1) out vec4 out_Albedo;
layout(location = 2) out vec4 out_Normal;
layout(location = 3) out float out_Depth;
layout(location = 4) out vec2 out_MotionVec;
layout(location = 5) out vec4 out_Metallic;
layout(location = 6) out vec4 out_Roughness;

layout(std140, binding = 2) uniform UniformBuffer {
    mat4 WVP;
    mat4 World;
    mat4 prevWVP;
    vec4 Ka_Ni;
    vec4 Kd_Ns;
    vec4 Ks_d;
    vec4 Ke_Tf;
    vec4 Tf;
} ubo;

layout(binding = 3) uniform sampler2D texSampler;
layout(binding = 7) uniform sampler2D metallicSampler;
layout(binding = 8) uniform sampler2D roughnessSampler;
layout(binding = 9) uniform sampler2D opacitySampler;

float linearizeDepth(float depth) {
    float zNear = 0.1;
    float zFar  = 1000.0;
    float z_ndc = depth * 2.0 - 1.0;  // [0,1] -> [-1,1]
    return (2.0 * zNear * zFar) / (zFar + zNear - z_ndc * (zFar - zNear));
}

void main()
{
    vec4 texVal = texture(texSampler, texCoord);
    vec3 baseColor = texVal.rgb * ubo.Kd_Ns.xyz;
    vec3 metallicColor = texture(metallicSampler, texCoord).rgb;
    vec3 roughnessColor = texture(roughnessSampler, texCoord).rgb;

    float opacityMap = texture(opacitySampler, texCoord).r;
    float alpha = texVal.a * opacityMap * ubo.Ks_d.w;

    if (alpha < 0.1) discard;

    vec3 N = normalize(v_Normal);
    float rawDepth = gl_FragCoord.z;
    float linDepth = linearizeDepth(rawDepth);

    vec2 currentNDC = vCurrentClipPos.xy / vCurrentClipPos.w;
    vec2 prevNDC = vPrevClipPos.xy / vPrevClipPos.w;
    vec2 currentUV = currentNDC * 0.5 + 0.5;
    vec2 prevUV = prevNDC * 0.5 + 0.5;
    vec2 motion = currentUV - prevUV;

    out_Pos = vec4(fragPos, 1.0);
    out_Albedo = vec4(baseColor, alpha);
    out_Normal = vec4(N, roughnessColor.r);
    out_Depth = linDepth;
    out_MotionVec = motion;
    out_Metallic = vec4(metallicColor, 1.0);
    out_Roughness = vec4(roughnessColor, 1.0);
}
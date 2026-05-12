#version 460
#extension GL_EXT_nonuniform_qualifier : enable

struct VertexData {
    float pos_x, pos_y, pos_z;
    float u, v;
    float normal_x, normal_y, normal_z;
    float tangent_x, tangent_y, tangent_z;
    float bitangent_x, bitangent_y, bitangent_z;
};

layout(std430, binding = 0) readonly buffer Vertices { VertexData v[]; } in_Vertices;
layout(std430, binding = 1) readonly buffer Indices  { int i[]; } in_Indices;

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

layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 v_Normal;
layout(location = 5) out vec4 vCurrentClipPos;
layout(location = 6) out vec4 vPreviousClipPos;

void main()
{
    int Index = in_Indices.i[gl_VertexIndex];
    VertexData vtx = in_Vertices.v[Index];

    vec3 pos       = vec3(vtx.pos_x, vtx.pos_y, vtx.pos_z);
    vec3 normal    = vec3(vtx.normal_x, vtx.normal_y, vtx.normal_z);

    vec3 worldPos = vec3(ubo.World * vec4(pos, 1.0));
    mat3 normalMatrix = mat3(transpose(inverse(ubo.World)));

    vec3 N = normalize(normalMatrix * normal);

    fragPos = worldPos;
    texCoord = vec2(vtx.u, vtx.v);
    v_Normal = N;

    gl_Position = ubo.WVP * vec4(pos, 1.0);
    vCurrentClipPos = gl_Position;
    vPreviousClipPos = ubo.prevWVP * vec4(pos, 1.0);
}
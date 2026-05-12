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

layout(location = 0) out vec2  texCoord;
layout(location = 1) out vec3  fragPos;
layout(location = 2) out mat3  TBN;

layout(location = 5) out vec4  vCurrentClipPos;
layout(location = 6) out vec4  vPreviousClipPos;

void main()
{
    int Index = in_Indices.i[gl_VertexIndex];
    VertexData vtx = in_Vertices.v[Index];

    vec3 pos       = vec3(vtx.pos_x,       vtx.pos_y,       vtx.pos_z);
    vec3 normal    = vec3(vtx.normal_x,    vtx.normal_y,    vtx.normal_z);
    vec3 tangent   = vec3(vtx.tangent_x,   vtx.tangent_y,   vtx.tangent_z);
    vec3 bitangent = vec3(vtx.bitangent_x, vtx.bitangent_y, vtx.bitangent_z);

    vec3 worldPos = vec3(ubo.World * vec4(pos, 1.0));

    mat3 normalMatrix = transpose(inverse(mat3(ubo.World)));
    mat3 worldMat3 = mat3(ubo.World);

    vec3 N = normalize(normalMatrix * normal);

    vec3 raw_T = worldMat3 * tangent;
    vec3 raw_B = worldMat3 * bitangent;

    vec3 T, B;

    if (dot(raw_T, raw_T) < 0.0001) {
        vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
        T = normalize(cross(up, N));
        B = cross(N, T);
    } else {
        T = normalize(raw_T);
        B = normalize(raw_B);

        // Gram-Schmidt orthogonalization
        T = normalize(T - dot(T, N) * N);

        // Handedness fix: Use < 0.0 instead of sign() to prevent
        // multiplying by 0 if the original vectors were malformed.
        vec3 B_cross = cross(N, T);
        float handedness = dot(B, B_cross) < 0.0 ? -1.0 : 1.0;
        B = B_cross * handedness;
    }

    TBN = mat3(T, B, N);

    fragPos  = worldPos;
    texCoord = vec2(vtx.u, vtx.v);

    gl_Position      = ubo.WVP * vec4(pos, 1.0);
    vCurrentClipPos  = gl_Position;
    vPreviousClipPos = ubo.prevWVP * vec4(pos, 1.0);
}
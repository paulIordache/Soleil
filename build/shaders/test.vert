
#version 460

struct VertexData
{
    float pos_x, pos_y, pos_z;
    float u, v;
    float normal_x, normal_y, normal_z;
    float tangent_x, tangent_y, tangent_z;
    float bitangent_x, bitangent_y, bitangent_z;
};

layout (std430, binding = 0) readonly buffer Vertices { VertexData v[]; } in_Vertices;

layout (binding = 1) readonly buffer Indices { int i[]; } in_Indices;

layout (binding = 2) readonly uniform UniformBuffer { mat4 WVP; } ubo;

layout(location = 0) out vec2 texCoord;

void main()
{
    int Index = in_Indices.i[gl_VertexIndex];

    VertexData vtx = in_Vertices.v[Index];

    vec3 pos = vec3(vtx.pos_x, vtx.pos_y, vtx.pos_z);

    gl_Position = ubo.WVP * vec4(pos, 1.0);

    texCoord = vec2(vtx.u, vtx.v);
}

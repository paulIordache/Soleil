#version 460

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 out_Color;

layout(binding = 3) uniform sampler2D texSampler;

void main()
{
    out_Color = texture(texSampler, texCoord);
    //out_Color = vec4(1.0);
}
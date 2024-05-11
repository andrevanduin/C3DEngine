#version 450

layout(set = 1, binding = 1) uniform sampler2D colorMap;

layout(location = 1) in struct Dto
{
    vec2 texCoord;
} inDto;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(1.0);
}
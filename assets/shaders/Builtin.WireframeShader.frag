#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in struct dto
{
    vec4 color;
} inDto;

void main()
{
    outColor = inDto.color;
}
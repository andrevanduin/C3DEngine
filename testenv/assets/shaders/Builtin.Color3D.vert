#version 450

// NOTE: w is ignored.
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

layout(set = 0, binding = 0) uniform globalUniformObject
{
    mat4 projection;
    mat4 view;
} globalUbo;

layout(push_constant) uniform pushConstants
{
    mat4 model;
} localUbo;

layout(location = 1) out struct dto
{
    vec4 color;
} outDto;

void main()
{
    outDto.color = inColor;
    gl_Position = globalUbo.projection * globalUbo.view * localUbo.model * vec4(inPosition.xyz, 1.0);
}
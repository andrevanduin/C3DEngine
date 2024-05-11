#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec4 inTangent;

layout(set = 0, binding = 0) uniform globalUniformObject
{
    mat4 projection;
    mat4 view;
} globalUbo;

layout(set = 1, binding = 0) uniform instanceUniformObject
{
    vec4 padding;
} instanceUbo;

// Push constants are only guaranteed to be a total of 128 bytes.
layout(push_constant) uniform PushConstants 
{
	mat4 model; // 64 bytes
} localUbo;

layout(location = 1) out struct dto
{
    vec2 texCoord;   
} outDto;

void main()
{
    outDto.texCoord = inTexCoord;
    gl_Position = globalUbo.projection * globalUbo.view * localUbo.model * vec4(inPosition, 1.0);
}
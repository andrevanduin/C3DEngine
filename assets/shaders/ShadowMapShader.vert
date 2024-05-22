#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec4 inTangent;

#define MAX_CASCADES 4

layout(set = 0, binding = 0) uniform globalUniformObject
{
    mat4 projections[MAX_CASCADES];
    mat4 views[MAX_CASCADES];
} globalUbo;

// Push constants are only guaranteed to be a total of 128 bytes.
layout(push_constant) uniform PushConstants 
{
	mat4 model; // 64 bytes
    uint cascadeIndex;
} localUbo;

layout(location = 1) out struct dto
{
    vec2 texCoord;   
} outDto;

void main()
{
    outDto.texCoord = inTexCoord;
    gl_Position = globalUbo.projections[localUbo.cascadeIndex] * globalUbo.views[localUbo.cascadeIndex] * localUbo.model * vec4(inPosition, 1.0);
}
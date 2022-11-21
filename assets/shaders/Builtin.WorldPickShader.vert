#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec3 inTangent;

layout(set = 0, binding = 0) uniform globalUniformObject
{
	mat4 projection;
	mat4 view;

} globalUbo;

layout(push_constant) uniform pushConstants 
{
	mat4 model;
} uPushConstants;

void main()
{
	gl_Position = globalUbo.projection * globalUbo.view * uPushConstants.model * vec4(inPosition, 1.0);
}
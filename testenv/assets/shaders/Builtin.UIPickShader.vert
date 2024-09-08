#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

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
	gl_Position = globalUbo.projection * globalUbo.view * uPushConstants.model * vec4(inPosition, 0.0, 1.0);
}
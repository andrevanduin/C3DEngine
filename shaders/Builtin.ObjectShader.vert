
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform globalUniformObject 
{
	mat4 projection;
	mat4 view;
} globalUbo;

layout(push_constant) uniform pushConstants 
{
	// Push constants are only guaranteed to be a total of 128 bytes.
	mat4 model; // 64 bytes
} uPushConstants;

void main()
{
	gl_Position = globalUbo.projection * globalUbo.view * uPushConstants.model * vec4(inPosition, 1.0);
}
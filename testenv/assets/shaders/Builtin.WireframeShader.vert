
#version 450

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform globalUniformObject 
{
	mat4 projection;
	mat4 view;
} globalUbo;

layout(set = 1, binding = 0) uniform instanceUniformObject
{
    vec4 color;
} instanceUbo;

// Push constants are only guaranteed to be a total of 128 bytes.
layout(push_constant) uniform PushConstants 
{
	mat4 model; // 64 bytes
} uPushConstants;

// Data transfer object
layout(location = 0) out struct dto 
{
	vec4 color;
} outDto;

void main()
{
	outDto.color = instanceUbo.color;
	
	gl_Position = globalUbo.projection * globalUbo.view * uPushConstants.model * vec4(inPosition, 1.0);
}
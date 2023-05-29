
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(set = 0, binding = 0) uniform globalUniformObject 
{
	mat4 projection;
	mat4 view;
	vec3 viewPosition;
} globalUbo;

// Push constants are only guaranteed to be a total of 128 bytes.
layout(push_constant) uniform pushConstants 
{
	mat4 model; // 64 bytes
} uPushConstants;

// Data transfer object
layout(location = 1) out struct dto 
{
	vec4 color;
} outDto;

void main()
{
	// Simply pass color
	outDto.color = inColor;

	gl_Position = globalUbo.projection * globalUbo.view * uPushConstants.model * vec4(inPosition, 1.0);
}
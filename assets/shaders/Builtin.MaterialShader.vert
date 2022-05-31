
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(set = 0, binding = 0) uniform globalUniformObject 
{
	mat4 projection;
	mat4 view;
} globalUbo;

// Push constants are only guaranteed to be a total of 128 bytes.
layout(push_constant) uniform pushConstants 
{
	mat4 model; // 64 bytes
} uPushConstants;

layout(location = 0) out int outMode;

// Data transfer object
layout(location = 1) out struct dto 
{
	vec2 texCoord;
} outDto;

void main()
{
	outDto.texCoord = inTexCoord;
	gl_Position = globalUbo.projection * globalUbo.view * uPushConstants.model * vec4(inPosition, 1.0);
}
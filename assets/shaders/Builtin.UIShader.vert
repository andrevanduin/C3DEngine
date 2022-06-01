#version 450

layout(location = 0) in vec2 inPosition;
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
    // Intenionally flip y texture coordinate. This along with the flipped ortho matrix, puts [0, 0] in the top-left
    // instead of bottom-left and adjusts texture coordinates to show in the right direction
	outDto.texCoord = vec2(inTexCoord.x, 1.0 - inTexCoord.y);
	gl_Position = globalUbo.projection * globalUbo.view * uPushConstants.model * vec4(inPosition, 0.0, 1.0);
}
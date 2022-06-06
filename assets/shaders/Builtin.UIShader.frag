#version 450

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform localUniformObject
{
	vec4 diffuseColor;
} objectUbo;

// Samplers
const int SAMP_DIFFUSE = 0;
layout(set = 1, binding = 1) uniform sampler2D samplers[1];

layout(location = 1) in struct dto
{
	vec2 texCoord;
} inDto;

void main() 
{
	outColor = objectUbo.diffuseColor * texture(samplers[SAMP_DIFFUSE], inDto.texCoord);
}
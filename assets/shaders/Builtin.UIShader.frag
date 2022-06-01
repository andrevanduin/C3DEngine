#version 450

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform localUniformObject
{
	vec4 diffuseColor;
} objectUbo;

// Samplers
layout(set = 1, binding = 1) uniform sampler2D diffuseSampler;

layout(location = 1) in struct dto
{
	vec2 texCoord;
} inDto;

void main() 
{
	outColor = objectUbo.diffuseColor * texture(diffuseSampler, inDto.texCoord);
}
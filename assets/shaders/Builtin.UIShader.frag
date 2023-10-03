#version 450

layout(location = 0) out vec4 outColor;

struct UIProperties 
{
	vec4 diffuseColor;
};

layout(set = 1, binding = 0) uniform localUniformObject
{
	UIProperties properties;
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
	outColor = objectUbo.properties.diffuseColor * texture(samplers[SAMP_DIFFUSE], inDto.texCoord);
}
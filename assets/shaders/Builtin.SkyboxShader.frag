#version 450

layout(location = 0) in vec3 texCoord;
layout(location = 0) out vec4 outColor;

// Samplers
const int SAMP_DIFFUSE = 0;
layout(set = 1, binding = 0) uniform samplerCube samplers[1];

void main() 
{
	outColor = texture(samplers[SAMP_DIFFUSE], texCoord);
}
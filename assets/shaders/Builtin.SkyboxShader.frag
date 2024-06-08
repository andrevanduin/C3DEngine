#version 450

layout(location = 0) in vec3 texCoord;
layout(location = 0) out vec4 outColor;

// Samplers
layout(set = 1, binding = 0) uniform samplerCube cubeTexture;

void main() 
{
	outColor = texture(cubeTexture, texCoord);
}
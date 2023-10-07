#version 450

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform localUniformObject 
{
	vec3 idColor;
} objectUbo;

void main()
{
	outColor = vec4(objectUbo.idColor, 1.0);
}
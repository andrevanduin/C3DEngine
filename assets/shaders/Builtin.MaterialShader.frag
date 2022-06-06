#version 450

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform localUniformObject
{
	vec4 diffuseColor;
	float shininess;
} objectUbo;

struct DirectionalLight 
{
	vec3 direction;
	vec4 color;
};

// TODO: feed in from cpu instead of hardcoding
DirectionalLight dirLight = 
{
	vec3(-0.57735, -0.57735, -0.57735),
	vec4(0.8, 0.8, 0.8, 1.0)
};

// Samplers, diffuse, spec
const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL = 2;
layout(set = 1, binding = 1) uniform sampler2D samplers[3];

layout(location = 1) in struct dto
{
	vec4 ambientColor;
	vec2 texCoord;
	vec3 normal;
	vec3 viewPosition;
	vec3 fragPosition;
	vec4 color;
	vec4 tangent;
} inDto;

// Matrix to take normals from texture space to world space
// Made up of:
// 	tangent
// 	biTangent
// 	normal
mat3 TBN;

vec4 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDirection);

void main() 
{
	vec3 normal = inDto.normal;
	vec3 tangent = inDto.tangent.xyz;
	tangent = (tangent - dot(tangent, normal) * normal);
	vec3 biTangent = cross(inDto.normal, inDto.tangent.xyz) * inDto.tangent.w;
	TBN = mat3(tangent, biTangent, normal);

	// Update the normal to use a sample from the normal map
	vec3 localNormal = 2.0 * texture(samplers[SAMP_NORMAL], inDto.texCoord).rgb - 1.0;
	normal = normalize(TBN * localNormal);

	vec3 viewDirection = normalize(inDto.viewPosition - inDto.fragPosition);
	outColor = CalculateDirectionalLight(dirLight, normal, viewDirection);
}

vec4 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDirection) 
{
	float diffuseFactor = max(dot(normal, -light.direction), 0.0);

	vec3 halfDirection = normalize(viewDirection - light.direction);
	float specularFactor = pow(max(dot(halfDirection, normal), 0.0), objectUbo.shininess);

	vec4 diffSamp = texture(samplers[SAMP_DIFFUSE], inDto.texCoord);

	vec4 ambient = vec4(vec3(inDto.ambientColor * objectUbo.diffuseColor), diffSamp.a);
	vec4 diffuse = vec4(vec3(light.color * diffuseFactor), diffSamp.a);
	vec4 specular = vec4(vec3(light.color * specularFactor), diffSamp.a);

	diffuse *= diffSamp;
	ambient *= diffSamp;
	specular *= vec4(texture(samplers[SAMP_SPECULAR], inDto.texCoord).rgb, diffuse.a);

	return (ambient + diffuse + specular);
}
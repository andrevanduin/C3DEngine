#version 450
// Builtin.TerrainShader.frag

layout(location = 0) out vec4 outColor;

struct DirectionalLight 
{
	vec4 color;
	vec4 direction;
};

struct PointLight 
{
	vec4 color;
	vec4 position;
	// Usually 1, make sure denominator never gets smaller than 1
	float fConstant;
	// Reduces light intensity linearly
	float linear;
	// Makes the light fall of slower at longer dinstances
	float quadratic;
	float padding;
};

const int POINT_LIGHT_MAX = 10;
const int MAX_MATERIALS = 4;

layout(set = 0, binding = 0) uniform globalUniformObject
{
	mat4 projection;
	mat4 view;
	vec4 ambientColor;
	vec3 viewPosition;
	int mode;
    DirectionalLight dirLight;
} globalUbo;

struct MaterialPhongProperties
{
	vec4 diffuseColor;
	vec3 padding;
	float shininess;
};

struct MaterialTerrainProperties
{
	MaterialPhongProperties materials[MAX_MATERIALS];
	vec3 padding;
	int numMaterials;
	vec4 padding2;
};

layout(set = 1, binding = 0) uniform instanceUniformObject
{
	MaterialTerrainProperties properties;
	PointLight pLights[POINT_LIGHT_MAX];
	vec3 padding;
	int numPLights;
} instanceUbo;

// Samplers, Diffuse, Specular and Normal
const int SAMP_DIFFUSE_OFFSET = 0;
const int SAMP_SPECULAR_OFFSET = 1;
const int SAMP_NORMAL_OFFSET = 2;

// Diffuse, Specular, Normal, Diffuse, Specular, Normal, etc...
layout(set = 1, binding = 1) uniform sampler2D samplers[3 * MAX_MATERIALS];

layout(location = 0) flat in int inMode;

layout(location = 1) in struct dto
{
	vec4 ambientColor;
	vec2 texCoord;
	vec3 normal;
	vec3 viewPosition;
	vec3 fragPosition;
	vec4 color;
	vec3 tangent;
	vec4 materialWeights;
} inDto;

// Matrix to take normals from texture space to world space
// Made up of:
// 	tangent
// 	biTangent
// 	normal
mat3 TBN;

vec4 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDirection, vec4 diffuseSamp, vec4 specularSamp, MaterialPhongProperties materialInfo);
vec4 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPosition, vec3 viewDirection, vec4 diffuseSamp, vec4 specularSamp, MaterialPhongProperties materialInfo);

void main() 
{
	if (inMode == 2)
	{
		outColor = vec4(abs(inDto.normal), 1.0);
		return;
	}

	vec3 tangent = (inDto.tangent - dot(inDto.tangent, inDto.normal) * inDto.normal);
	vec3 biTangent = cross(inDto.normal, inDto.tangent);
	TBN = mat3(inDto.tangent, biTangent, inDto.normal);

	vec4 diffuses[MAX_MATERIALS];
	vec4 speculars[MAX_MATERIALS];
	vec3 normals[MAX_MATERIALS];
	
	MaterialPhongProperties mat;
	mat.diffuseColor = vec4(0);
	mat.shininess = 1.0;

	vec4 diffuse = vec4(1);
	vec4 specular = vec4(0.5);
	vec3 normal = vec3(0, 0, 1);

	for (int m = 0; m < instanceUbo.properties.numMaterials; ++m)
	{
		diffuses[m] = texture(samplers[(m * 3) + SAMP_DIFFUSE_OFFSET], inDto.texCoord);
		speculars[m] = texture(samplers[(m * 3) + SAMP_SPECULAR_OFFSET], inDto.texCoord);
		normals[m] = normalize(TBN * (2.0 * texture(samplers[(m * 3) + SAMP_NORMAL_OFFSET], inDto.texCoord).rgb - 1.0));

		diffuse = mix(diffuse, diffuses[m], inDto.materialWeights[m]);
		specular = mix(specular, speculars[m], inDto.materialWeights[m]);
		normal = mix(normal, normals[m], inDto.materialWeights[m]);

		mat.diffuseColor = mix(mat.diffuseColor, instanceUbo.properties.materials[m].diffuseColor, inDto.materialWeights[m]);
		mat.shininess = mix(mat.shininess, instanceUbo.properties.materials[m].shininess, inDto.materialWeights[m]);
	}

	vec3 viewDirection = normalize(inDto.viewPosition - inDto.fragPosition);
	outColor = CalculateDirectionalLight(globalUbo.dirLight, normal, viewDirection, diffuse, specular, mat);

	for (int i = 0; i < instanceUbo.numPLights; i++)
	{
		outColor += CalculatePointLight(instanceUbo.pLights[i], normal, inDto.fragPosition, viewDirection, diffuse, specular, mat);
	}
}

vec4 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDirection, vec4 diffuseSamp, vec4 specularSamp, MaterialPhongProperties materialInfo) 
{
	float diffuseFactor = max(dot(normal, -light.direction.xyz), 0.0);

	vec3 halfDirection = normalize(viewDirection - light.direction.xyz);
	float specularFactor = pow(max(dot(halfDirection, normal), 0.0), materialInfo.shininess);

	vec4 ambient = vec4(vec3(inDto.ambientColor * materialInfo.diffuseColor), diffuseSamp.a);
	vec4 diffuse = vec4(vec3(light.color * diffuseFactor), diffuseSamp.a);
	vec4 specular = vec4(vec3(light.color * specularFactor), diffuseSamp.a);

	if (inMode == 0) 
	{
		diffuse *= diffuseSamp;
		ambient *= diffuseSamp;
		specular *= specularSamp;
	}

	return (ambient + diffuse + specular);
}

vec4 CalculatePointLight(PointLight light, vec3 normal, vec3 fragPosition, vec3 viewDirection, vec4 diffuseSamp, vec4 specularSamp, MaterialPhongProperties materialInfo)
{
	vec3 lightDirection = normalize(light.position.xyz - fragPosition);
	float diff = max(dot(normal, lightDirection), 0.0);

	vec3 reflectDirection = reflect(-lightDirection, normal);
	float spec = pow(max(dot(viewDirection, reflectDirection), 0.0), materialInfo.shininess);

	// Calculate attenuation, or light falloff over distance
	float distance = length(light.position.xyz - fragPosition);
	float attenuation = 1.0 / (light.fConstant + light.linear * distance + light.quadratic * (distance * distance));

	vec4 ambient = inDto.ambientColor;
	vec4 diffuse = light.color * diff;
	vec4 specular = light.color * spec;

	if (inMode == 0) 
	{
		diffuse *= diffuseSamp;
		ambient *= diffuseSamp;
		specular *= specularSamp;
	}

	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

	return (ambient + diffuse + specular);
}
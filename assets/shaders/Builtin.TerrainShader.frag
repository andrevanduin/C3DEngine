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

// Samplers, albedo, normal, metallic, roughness and ao
const int SAMP_ALBEDO_OFFSET = 0;
const int SAMP_NORMAL_OFFSET = 1;
const int SAMP_METALLIC_OFFSET = 2;
const int SAMP_ROUGHNESS_OFFSET = 3;
const int SAMP_AO_OFFSET = 4;
// Irradiance cube comes after all material textures
const int SAMP_IRRADIANCE_CUBE = 5 * MAX_MATERIALS;

const float PI = 3.14159265359;

// albedo, normal, metallic, roughness, ao, etc...
layout(set = 1, binding = 1) uniform sampler2D samplers[(5 * MAX_MATERIALS)];
// IBL - alias to get cube samplers from the same samplers array
layout(set = 1, binding = 1) uniform samplerCube cubeSamplers [1 + (5 * MAX_MATERIALS)];

layout(location = 0) flat in int inMode;

layout(location = 1) in struct dto
{
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

// This is based off the Cook-Torrance BRDF (Bidirectional Reflective Distribution Function).
// Which uses a micro-facet model to use roughness and metallic properties of materials to produce a physically accurate representation of material refelectance.
// See: https://graphicscompendium.com/gamedev/15-pbr

float GeometrySchlickGGX(float normalDotDirection, float roughness)
{
    roughness += 1.0;
    float k = (roughness * roughness) / 8.0;
    return normalDotDirection / (normalDotDirection * (1.0 - k) + k);
}

vec3 CalculateDirectionalLightRadiance(DirectionalLight light, vec3 viewDirection);
vec3 CalculateReflectance(vec3 albedo, vec3 normal, vec3 viewDirection, vec3 lightDirection, float metallic, float roughness, vec3 baseReflectivity, vec3 radiance);
vec3 CalculatePointLightRadiance(PointLight light, vec3 viewDirection, vec3 fragPositionXyz);

void main() 
{
    vec3 normal = inDto.normal;
    vec3 tangent = inDto.tangent;
    tangent = (tangent - dot(tangent, normal) * normal);
    vec3 biTangent = cross(inDto.normal, inDto.tangent);
    TBN = mat3(tangent, biTangent, normal);

    vec3 normals[MAX_MATERIALS];
    vec4 albedos[MAX_MATERIALS];
    vec4 metallics[MAX_MATERIALS];
    vec4 roughnesses[MAX_MATERIALS];
    vec4 aos[MAX_MATERIALS];

    // Sample each material
    for (int m = 0; m < instanceUbo.properties.numMaterials; ++m)
    {
        int mElement = (m * 5);

        albedos[m] = texture(samplers[mElement + SAMP_ALBEDO_OFFSET], inDto.texCoord);
        albedos[m] = vec4(pow(albedos[m].rgb, vec3(2.2)), albedos[m].a);

        vec3 localNormal = 2.0 * texture(samplers[mElement + SAMP_NORMAL_OFFSET], inDto.texCoord).rgb - 1.0;
        normals[m] = normalize(TBN * localNormal);

        metallics[m] = texture(samplers[mElement + SAMP_METALLIC_OFFSET], inDto.texCoord); 
        roughnesses[m] = texture(samplers[mElement + SAMP_ROUGHNESS_OFFSET], inDto.texCoord);
        aos[m] = texture(samplers[mElement + SAMP_AO_OFFSET], inDto.texCoord);
    }

    vec4 albedo =
        albedos[0] * inDto.materialWeights[0] +
        albedos[1] * inDto.materialWeights[1] +
        albedos[2] * inDto.materialWeights[2] +
        albedos[3] * inDto.materialWeights[3];

    // Ensure our albedo is fully opaque so we don't get transparent terrains
    albedo.a = 1.0;

    normal = 
        normals[0] * inDto.materialWeights[0] +
        normals[1] * inDto.materialWeights[1] +
        normals[2] * inDto.materialWeights[2] +
        normals[3] * inDto.materialWeights[3];

    float metallic = 
        metallics[0].r * inDto.materialWeights[0] +
        metallics[1].r * inDto.materialWeights[1] +
        metallics[2].r * inDto.materialWeights[2] +
        metallics[3].r * inDto.materialWeights[3];

    float roughness = 
        roughnesses[0].r * inDto.materialWeights[0] +
        roughnesses[1].r * inDto.materialWeights[1] +
        roughnesses[2].r * inDto.materialWeights[2] +
        roughnesses[3].r * inDto.materialWeights[3];
    
    float ao = 
        aos[0].r * inDto.materialWeights[0] +
        aos[1].r * inDto.materialWeights[1] +
        aos[2].r * inDto.materialWeights[2] +
        aos[3].r * inDto.materialWeights[3];
	
	// Calculate reflectance at normal incidence; if dia-electric (plastic-like) use baseReflectivity
    // of 0.04 and if it's a metal, use the albedo color as baseReflectivity.
    vec3 baseReflectivity = vec3(0.04);
    baseReflectivity = mix(baseReflectivity, albedo.xyz, metallic);

	if (inMode == 0 || inMode == 1)
	{
		vec3 viewDirection = normalize(inDto.viewPosition - inDto.fragPosition);

		// Don't include albedo in inMode == 1 (lighting-only) by making it pure white.
		albedo.xyz += (vec3(1.0) * inMode);
		albedo.xyz = clamp(albedo.xyz, vec3(0.0), vec3(1.0));

		// Overall reflectance.
		vec3 totalReflectance = vec3(0.0);

		// Directional light radiance.
		{
			DirectionalLight light = globalUbo.dirLight;
			vec3 lightDirection = normalize(-light.direction.xyz);
			vec3 radiance = CalculateDirectionalLightRadiance(light, viewDirection);

			totalReflectance += CalculateReflectance(albedo.xyz, normal, viewDirection, lightDirection, metallic, roughness, baseReflectivity, radiance);
		}

		// Point light radiance
		for (int i = 0; i < instanceUbo.numPLights; ++i)
		{
			PointLight light = instanceUbo.pLights[i];
			vec3 lightDirection = normalize(light.position.xyz - inDto.fragPosition.xyz);
			vec3 radiance = CalculatePointLightRadiance(light, viewDirection, inDto.fragPosition.xyz);

			totalReflectance += CalculateReflectance(albedo.xyz, normal, viewDirection, lightDirection, metallic, roughness, baseReflectivity, radiance);
		}

        // Irradiance holds all the scene's indirect diffuse light. Use the surface normal to sample from it.
        vec3 irradiance = texture(cubeSamplers[SAMP_IRRADIANCE_CUBE], normal).rgb;

		// Combine irradiance with albedo and ambient occlusion. Also add the total reflectance that we have accumulated.
		vec3 ambient = irradiance * albedo.xyz * ao;
		vec3 color = ambient + totalReflectance;

		// HDR tonemapping
		color = color / (color + vec3(1.0));
		// Gamma correction
		color = pow(color, vec3(1.0 / 2.2));

		// Ensure the alpha is based on the albedo's original alpha value.
		outColor = vec4(color, albedo.a);
	}
	else // inMode == 2 (normals-only)
	{
		outColor = vec4(abs(normal), 1.0);
	}
}

vec3 CalculateReflectance(vec3 albedo, vec3 normal, vec3 viewDirection, vec3 lightDirection, float metallic, float roughness, vec3 baseReflectivity, vec3 radiance)
{
    vec3 halfway = normalize(viewDirection + lightDirection);

    // Normal distribution - approximate the amount of the surface's micro-facets that are aligned to the halfway vector.
    // This is directly influenced by the roughness of the surface. More aligned micro-facets == more shiny, less == more dull / less reflection.
    float roughnessPow2 = roughness * roughness;
	float roughnessPow4 = roughnessPow2 * roughnessPow2;
    float normalDotHalfway = max(dot(normal, halfway), 0.0);
    float normalDotHalfwaySq = normalDotHalfway * normalDotHalfway;
    float denom = (normalDotHalfwaySq * (roughnessPow4 - 1.0) + 1.0);
    denom = PI * denom * denom;
    float normalDistribution = (roughnessPow4 / denom);

    // Geometry function which calculates self-shadowing on micro-facets (which is more prenouced on rough surfaces).
    float normalDotViewDirection = max(dot(normal, viewDirection), 0.0);
    // Scale the light by the dot product of normal and lightDirection.
    float normalDotLightDirection = max(dot(normal, lightDirection), 0.0);
    float ggx0 = GeometrySchlickGGX(normalDotViewDirection, roughness);
    float ggx1 = GeometrySchlickGGX(normalDotLightDirection, roughness);
    float geometry = ggx1 * ggx0;

    // Fresnel-Schlick approximation for the fresnel. This generates a ratio of surface reflection at different surface angles.
    // In many cases, reflectivity can be higher at more extreme angles.
    float cosTheta = max(dot(halfway, viewDirection), 0.0);
    vec3 fresnel = baseReflectivity + (1.0 - baseReflectivity) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);

    // Calculate our specular reflection
    vec3 numerator = normalDistribution * geometry * fresnel;
    float denominator = 4.0 * max(dot(normal, viewDirection), 0.0) + 0.0001; // Prevent divide by 0
    vec3 specular = numerator / denominator;

    // For energy conservation, the diffuse and specular light can't be above 1.0 (unless our surface emits light).
    // To preserve this relationship the diffuse component should equal 1.0 - fresnel.
    vec3 refractionDiffuse = vec3(1.0) - fresnel;
    // Multiply by the inverse metalness such that only non-metals have diffuse lighting, or a linear blend if partly metal.
    refractionDiffuse *= 1.0 - metallic;

    // The end result is the reflectance to be added to the overall
    return (refractionDiffuse * albedo / PI + specular) * radiance * normalDotLightDirection;
}

vec3 CalculatePointLightRadiance(PointLight light, vec3 viewDirection, vec3 fragPositionXyz)
{
    // Per-light radiance based on the point light's attenuation
    float distance = length(light.position.xyz - fragPositionXyz);
    float attenuation = 1.0 / (light.fConstant + light.linear * distance + light.quadratic * (distance * distance));
    return light.color.rgb * attenuation;
}

vec3 CalculateDirectionalLightRadiance(DirectionalLight light, vec3 viewDirection)
{
    // For directional lights, radiance is just the same as it's light color
    return light.color.rgb;
}
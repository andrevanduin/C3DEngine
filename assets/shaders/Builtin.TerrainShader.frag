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
const int MAX_SHADOW_CASCADES = 4;

layout(set = 0, binding = 0) uniform globalUniformObject
{
	mat4 projection;
	mat4 view;
    mat4 lightSpace[MAX_SHADOW_CASCADES];
    vec4 ambientColor;
    DirectionalLight dirLight;
	vec3 viewPosition;
	int mode;
    int usePcf;
    float bias;
    vec2 padding;
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

// Material texture indices
const int SAMP_ALBEDO_OFFSET = 0;
const int SAMP_NORMAL_OFFSET = 1;
// Combination of metallic, roughness and ao
const int SAMP_COMBINED_OFFSET = 2;

const float PI = 3.14159265359;

// Material textures
layout(set = 1, binding = 1) uniform sampler2DArray materialTextures;
// Shadow maps
layout(set = 1, binding = 2) uniform sampler2DArray shadowTexture;
// Irradiance map
layout(set = 1, binding = 3) uniform samplerCube irradianceTexture;

layout(location = 0) flat in int inMode;
layout(location = 1) flat in int usePCF;

layout(location = 2) in struct dto
{
    vec4 lightSpaceFragPosition[MAX_SHADOW_CASCADES];
    vec4 cascadeSplits;
	vec2 texCoord;
	vec3 normal;
	vec3 viewPosition;
	vec3 fragPosition;
	vec4 color;
	vec3 tangent;
	vec4 materialWeights;
    float bias;
    vec3 padding;
} inDto;

// Matrix to take normals from texture space to world space
// Made up of:
// 	tangent
// 	biTangent
// 	normal
mat3 TBN;

float CalculateShadow(vec4 lightSpaceFragPosition, int cascadeIndex);

// This is based off the Cook-Torrance BRDF (Bidirectional Reflective Distribution Function).
// Which uses a micro-facet model to use roughness and metallic properties of materials to produce a physically accurate representation of material refelectance.
// See: https://graphicscompendium.com/gamedev/15-pbr
float GeometrySchlickGGX(float normalDotDirection, float roughness);
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
    float metallics[MAX_MATERIALS];
    float roughnesses[MAX_MATERIALS];
    float aos[MAX_MATERIALS];

    // Sample each material
    for (int m = 0; m < instanceUbo.properties.numMaterials; ++m)
    {
        int mElement = (m * 3);

        albedos[m] = texture(materialTextures, vec3(inDto.texCoord, mElement + SAMP_ALBEDO_OFFSET));
        albedos[m] = vec4(pow(albedos[m].rgb, vec3(2.2)), albedos[m].a);

        vec3 localNormal = 2.0 * texture(materialTextures, vec3(inDto.texCoord, mElement + SAMP_NORMAL_OFFSET)).rgb - 1.0;
        normals[m] = normalize(TBN * localNormal);
        
        vec4 combined = texture(materialTextures, vec3(inDto.texCoord, mElement + SAMP_COMBINED_OFFSET));
        metallics[m] = combined.r;
        roughnesses[m] = combined.g;
        aos[m] = combined.b;
    }

    // Mix the materials proportionally by their weight
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
        metallics[0] * inDto.materialWeights[0] +
        metallics[1] * inDto.materialWeights[1] +
        metallics[2] * inDto.materialWeights[2] +
        metallics[3] * inDto.materialWeights[3];

    float roughness =
        roughnesses[0] * inDto.materialWeights[0] +
        roughnesses[1] * inDto.materialWeights[1] +
        roughnesses[2] * inDto.materialWeights[2] +
        roughnesses[3] * inDto.materialWeights[3];
    
    float ao =
        aos[0] * inDto.materialWeights[0] +
        aos[1] * inDto.materialWeights[1] +
        aos[2] * inDto.materialWeights[2] +
        aos[3] * inDto.materialWeights[3];
	
    // Generate shadow value based on current fragment position against the shadow map
    vec4 fragPositionViewSpace = globalUbo.view * vec4(inDto.fragPosition, 1.0f);
    float depth = abs(fragPositionViewSpace).z;
    // Get the cascade index from the current fragment's position
    int cascadeIndex = MAX_SHADOW_CASCADES;
    for (int i = 0; i < MAX_SHADOW_CASCADES; ++i)
    {
        if (depth < inDto.cascadeSplits[i])
        {
            cascadeIndex = i;
            break;
        }
    }
    float shadow = CalculateShadow(inDto.lightSpaceFragPosition[cascadeIndex], cascadeIndex);

	// Calculate reflectance at normal incidence; if dia-electric (plastic-like) use baseReflectivity
    // of 0.04 and if it's a metal, use the albedo color as baseReflectivity.
    vec3 baseReflectivity = vec3(0.04);
    baseReflectivity = mix(baseReflectivity, albedo.xyz, metallic);

	if (inMode == 0 || inMode == 1 || inMode == 3)
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

			totalReflectance += (shadow * CalculateReflectance(albedo.xyz, normal, viewDirection, lightDirection, metallic, roughness, baseReflectivity, radiance));
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
        vec3 irradiance = texture(irradianceTexture, normal).rgb;

		// Combine irradiance with albedo and ambient occlusion. Also add the total reflectance that we have accumulated.
		vec3 ambient = irradiance * albedo.xyz * ao;

		vec3 color = ambient + totalReflectance;

		// HDR tonemapping
		color = color / (color + vec3(1.0));
		// Gamma correction
		color = pow(color, vec3(1.0 / 2.2));

        if (inMode == 3) // inMode == Cascades
        {
            switch (cascadeIndex)
            {
                case 0:
                    color *= vec3(1.0, 0.25, 0.25);
                    break;
                case 1:
                    color *= vec3(0.25, 1.0, 0.25);
                    break;
                case 2:
                    color *= vec3(0.25, 0.25, 1.0);
                    break;
                case 3:
                    color *= vec3(1.0, 1.0, 0.25);
                    break;
            }
        }

		// Ensure the alpha is based on the albedo's original alpha value.
		outColor = vec4(color, albedo.a);
	}
	else // inMode == 2 (normals-only)
	{
		outColor = vec4(abs(normal), 1.0);
	}
}

// Percentage-Closer Filtering
float CalculatePCF(vec3 projected, int cascadeIndex)
{
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowTexture, 0).xy;

    for (int x = -1; x <= 1; ++x) 
    {
        for(int y = -1; y <= 1; ++y) 
        {
            float pcfDepth = texture(shadowTexture, vec3(projected.xy + vec2(x, y) * texelSize, cascadeIndex)).r;
            shadow += projected.z - inDto.bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9;
    return 1.0 - shadow;
}

float CalculateUnfiltered(vec3 projected, int cascadeIndex) 
{
    // Sample the shadow map.
    float mapDepth = texture(shadowTexture, vec3(projected.xy, cascadeIndex)).r;

    // TODO: cast/get rid of branch.
    float shadow = projected.z - inDto.bias > mapDepth ? 0.0 : 1.0;
    return shadow;
}

float CalculateShadow(vec4 lightSpaceFragPosition, int cascadeIndex)
{
    // Perspective divide
    vec3 projected = lightSpaceFragPosition.xyz / lightSpaceFragPosition.w;
    // Reverse Y
    projected.y = 1.0 - projected.y;

    if (usePCF == 1)
    {
        return CalculatePCF(projected, cascadeIndex);
    }

    return CalculateUnfiltered(projected, cascadeIndex);
}

float GeometrySchlickGGX(float normalDotDirection, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    return normalDotDirection / (normalDotDirection * (1.0 - k) + k);
}

vec3 CalculateReflectance(vec3 albedo, vec3 normal, vec3 viewDirection, vec3 lightDirection, float metallic, float roughness, vec3 baseReflectivity, vec3 radiance)
{
    vec3 halfway = normalize(viewDirection + lightDirection);

    // Normal distribution - approximate the amount of the surface's micro-facets that are aligned to the halfway vector.
    // This is directly influenced by the roughness of the surface. More aligned micro-facets == more shiny, less == more dull / less reflection.
    float roughnessPow4 = roughness * roughness * roughness * roughness;
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
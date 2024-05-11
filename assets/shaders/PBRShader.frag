
#version 450

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

const int MAX_POINT_LIGHTS = 10;

struct PbrProperties
{
    vec4 diffuseColor;
    vec3 padding;
    float shininess;
};

layout(set = 1, binding = 0) uniform instanceUniformObject
{
    // TODO: Make global
	DirectionalLight dirLight;
	// TODO: Move after props
	PointLight pLights[MAX_POINT_LIGHTS];
	PbrProperties properties;
	uint numPLights;
} instanceUbo;

const int SAMP_ALBEDO = 0;
const int SAMP_NORMAL = 1;
const int SAMP_METALLIC = 2;
const int SAMP_ROUGHNESS = 3;
const int SAMP_AO = 4;
const int SAMP_SHADOW_MAP = 5;
const int SAMP_IBL_CUBE = 6;

const float PI = 3.14159265359;

// Samplers: albedo, normal, metallic, roughness, ao
layout(set = 1, binding = 1) uniform sampler2D samplers[7];
// IBL
layout(set = 1, binding = 1) uniform samplerCube cubeSamplers[7]; // Alias to get cube sampler

layout(location = 0) flat in int inMode;
layout(location = 1) flat in int usePCF;

// Data transfer object
layout(location = 2) in struct Dto
{
    vec4 lightSpaceFragPosition;
    vec4 ambient;
    vec2 texCoord;
    vec3 normal;
    vec3 viewPosition;
    vec3 fragPosition;
    vec4 color;
    vec3 tangent;
    float bias;
    vec3 padding;
} inDto;

mat3 TBN;

float CalculateShadow(vec4 lightSpaceFragPosition);

// This is based off the Cook-Torrance BRDF (Bidirectional Reflective Distribution Function).
// Which uses a micro-facet model to use roughness and metallic properties of materials to produce a physically accurate representation of material refelectance.
// See: https://graphicscompendium.com/gamedev/15-pbr
float GeometrySchlickGGX(float normalDotDirection, float roughness);
vec3 CalculateReflectance(vec3 albedo, vec3 normal, vec3 viewDirection, vec3 lightDirection, float metallic, float roughness, vec3 baseReflectivity, vec3 radiance);
vec3 CalculatePointLightRadiance(PointLight light, vec3 viewDirection, vec3 fragPositionXyz);
vec3 CalculateDirectionalLightRadiance(DirectionalLight light, vec3 viewDirection);

void main()
{
    vec3 normal = inDto.normal;
	vec3 tangent = inDto.tangent;
	tangent = (tangent - dot(tangent, normal) * normal);
	vec3 biTangent = cross(inDto.normal, inDto.tangent);
	TBN = mat3(tangent, biTangent, normal);

    // Update the normal to use a sample from the normal map.
	vec3 localNormal = 2.0 * texture(samplers[SAMP_NORMAL], inDto.texCoord).rgb - 1.0;
	normal = normalize(TBN * localNormal);

    vec4 albedoSamp = texture(samplers[SAMP_ALBEDO], inDto.texCoord);
    vec3 albedo = pow(albedoSamp.rgb, vec3(2.2));

    float metallic = texture(samplers[SAMP_METALLIC], inDto.texCoord).r;
    float roughness = texture(samplers[SAMP_ROUGHNESS], inDto.texCoord).r;
    float ao = texture(samplers[SAMP_AO], inDto.texCoord).r;

    // Calculate reflectance at normal incidence; if dia-electric (plastic-like) use baseReflectivity
    // of 0.04 and if it's a metal, use the albedo color as baseReflectivity.
    vec3 baseReflectivity = vec3(0.04);
    baseReflectivity = mix(baseReflectivity, albedo, metallic);

    if (inMode == 0 || inMode == 1) // (default or lighting-only mode)
    {
        vec3 viewDirection = normalize(inDto.viewPosition - inDto.fragPosition);

        // Don't include albedo in inMode == 1 (lighting-only) by making it pure white.
        albedo += (vec3(1.0) * inMode);
        albedo = clamp(albedo, vec3(0.0), vec3(1.0));

        // Overall reflectance.
        vec3 totalReflectance = vec3(0.0);

        // Directional light radiance.
        {
            DirectionalLight light = instanceUbo.dirLight;
            vec3 lightDirection = normalize(-light.direction.xyz);
            vec3 radiance = CalculateDirectionalLightRadiance(light, viewDirection);

            totalReflectance += CalculateReflectance(albedo, normal, viewDirection, lightDirection, metallic, roughness, baseReflectivity, radiance);
        }

        // Point light radiance
        for (int i = 0; i < instanceUbo.numPLights; ++i)
        {
            PointLight light = instanceUbo.pLights[i];
            vec3 lightDirection = normalize(light.position.xyz - inDto.fragPosition.xyz);
            vec3 radiance = CalculatePointLightRadiance(light, viewDirection, inDto.fragPosition.xyz);

            totalReflectance += CalculateReflectance(albedo, normal, viewDirection, lightDirection, metallic, roughness, baseReflectivity, radiance);
        }

        // Irradiance holds all the scene's indirect diffuse light. We use the surface normal to sample from it.
        vec3 irradiance = texture(cubeSamplers[SAMP_IBL_CUBE], normal).rgb;

        // Shadow
        float shadow = CalculateShadow(inDto.lightSpaceFragPosition);

        // Add in the albedo and ambient occlusion.
        vec3 ambient = irradiance * albedo * ao;
        
        vec3 color = ambient + totalReflectance * shadow;

        // HDR tonemapping
        color = color / (color + vec3(1.0));
        // Gamma correction
        color = pow(color, vec3(1.0 / 2.2));

        // Ensure the alpha is based on the albedo's original alpha value.
        outColor = vec4(color, albedoSamp.a);
    }
    else // inMode == 2 (normals-only mode)
    {
        outColor = vec4(abs(normal), 1.0);
    }
}

// Percentage-Closer Filtering
float CalculatePCF(vec3 projected)
{
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(samplers[SAMP_SHADOW_MAP], 0);

    for (int x = -1; x <= 1; ++x) 
    {
        for(int y = -1; y <= 1; ++y) 
        {
            float pcfDepth = texture(samplers[SAMP_SHADOW_MAP], projected.xy + vec2(x, y) * texelSize).r;
            shadow += projected.z - inDto.bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9;
    return 1.0 - shadow;
}

float CalculateUnfiltered(vec3 projected) 
{
    // Sample the shadow map.
    float mapDepth = texture(samplers[SAMP_SHADOW_MAP], projected.xy).r;

    // TODO: cast/get rid of branch.
    float shadow = projected.z - inDto.bias > mapDepth ? 0.0 : 1.0;
    return shadow;
}

float CalculateShadow(vec4 lightSpaceFragPosition)
{
    // Perspective divide
    vec3 projected = lightSpaceFragPosition.xyz / lightSpaceFragPosition.w;
    // Reverse Y
    projected.y = 1.0 - projected.y;

    if (usePCF == 1)
    {
        return CalculatePCF(projected);
    }

    return CalculateUnfiltered(projected);
}

float GeometrySchlickGGX(float normalDotDirection, float roughness)
{
    roughness += 1.0;
    float k = (roughness * roughness) / 8.0;
    return normalDotDirection / (normalDotDirection * (1.0 - k) + k);
}

vec3 CalculateReflectance(vec3 albedo, vec3 normal, vec3 viewDirection, vec3 lightDirection, float metallic, float roughness, vec3 baseReflectivity, vec3 radiance)
{
    vec3 halfway = normalize(viewDirection + lightDirection);

    // Normal distribution - approximate the amount of the surface's micro-facets that are aligned to the halfway vector.
    // This is directly influenced by the roughness of the surface. More aligned micro-facets == more shiny, less == more dull / less reflection.
    float roughnessPow4 = roughness * roughness * roughness * roughness;
    float normalDotHalfway = max(dot(normal, halfway), 0.0);
    float normalDotHalwaySq = normalDotHalfway * normalDotHalfway;
    float denom = (normalDotHalwaySq * (roughnessPow4 - 1.0) + 1.0);
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
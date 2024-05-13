
#version 450
// Builtin.TerrainShader.vert

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec4 inTangent;
// NOTE: Supports 4 materials max
layout(location = 5) in vec4 inMaterialWeights;

const int MAX_SHADOW_CASCADES = 4;

struct DirectionalLight 
{
    vec4 color;
    vec4 direction;
};

layout(set = 0, binding = 0) uniform globalUniformObject 
{
    mat4 projection;
    mat4 view;
    mat4 lightSpace[MAX_SHADOW_CASCADES];
    vec4 cascadeSplits;
    DirectionalLight dirLight;
    vec3 viewPosition;
    int mode;
    int usePCF;
    float bias;
    vec2 padding;
} globalUbo;

layout(push_constant) uniform PushConstants
{
    mat4 model;
} uPushConstants;

layout(location = 0) out int outMode;
layout(location = 1) out int usePCF;

// Data transfer object
layout(location = 2) out struct dto 
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
} outDto;

// Vulkan's Y axis is flipped and Z range is halved.
const mat4 bias = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

void main()
{
    outDto.texCoord = inTexCoord;
    outDto.color = inColor;
    outDto.materialWeights = inMaterialWeights;
    
    // Fragment position in world space
    outDto.fragPosition = vec3(uPushConstants.model * vec4(inPosition, 1.0));

    // Calculate mat3 version of our model
    mat3 m3Model = mat3(uPushConstants.model);
    // Convert local normal to "world space"
    outDto.normal = normalize(m3Model * inNormal);
    outDto.tangent = normalize(m3Model * inTangent.xyz);
    outDto.cascadeSplits = globalUbo.cascadeSplits;
    outDto.viewPosition = globalUbo.viewPosition;
    
    gl_Position = globalUbo.projection * globalUbo.view * uPushConstants.model * vec4(inPosition, 1.0);

    for (int i = 0; i < MAX_SHADOW_CASCADES; ++i)
    {
        outDto.lightSpaceFragPosition[i] = (bias * globalUbo.lightSpace[i]) * vec4(outDto.fragPosition, 1.0);
    }

    outMode = globalUbo.mode;
    usePCF = globalUbo.usePCF;
    outDto.bias = globalUbo.bias;
}
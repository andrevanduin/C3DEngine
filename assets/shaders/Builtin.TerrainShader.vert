
#version 450
// Builtin.TerrainShader.vert

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec4 inTangent;
// NOTE: Supports 4 materials max
layout(location = 5) in vec4 inMaterialWeights;

struct DirectionalLight 
{
    vec4 color;
    vec4 direction;
};

layout(set = 0, binding = 0) uniform globalUniformObject 
{
    mat4 projection;
    mat4 view;
    vec4 ambientColor;
    vec3 viewPosition;
    int mode;
    DirectionalLight dirLight;
} globalUbo;

layout(push_constant) uniform PushConstants
{
    mat4 model;
} uPushConstants;

layout(location = 0) out int outMode;

// Data transfer object
layout(location = 1) out struct dto 
{
    vec4 ambientColor;
    vec2 texCoord;
    vec3 normal;
    vec3 viewPosition;
    vec3 fragPosition;
    vec4 color;
    vec3 tangent;
    vec4 materialWeights;
} outDto;

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

    outDto.ambientColor = globalUbo.ambientColor;
    outDto.viewPosition = globalUbo.viewPosition;
    
    gl_Position = globalUbo.projection * globalUbo.view * uPushConstants.model * vec4(inPosition, 1.0);

    outMode = globalUbo.mode;
}
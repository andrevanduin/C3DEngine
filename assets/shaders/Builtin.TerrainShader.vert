
#version 450
// Builtin.TerrainShader.vert

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec3 inTangent;

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

layout(set = 0, binding = 0) uniform globalUniformObject 
{
    mat4 projection;
    mat4 view;
	mat4 model;
    vec4 ambientColor;
    vec3 viewPosition;
    int mode;
    vec4 diffuseColor;
    DirectionalLight dirLight;
    PointLight pLights[POINT_LIGHT_MAX];
    int numPLights;
    float shininess;
} globalUbo;

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
} outDto;

void main()
{
    outDto.texCoord = inTexCoord;
    outDto.color = inColor;
    
    // Fragment position in world space
    outDto.fragPosition = vec3(globalUbo.model * vec4(inPosition, 1.0));

    // Calculate mat3 version of our model
    mat3 m3Model = mat3(globalUbo.model);
    // Convert local normal to "world space"
    outDto.normal = normalize(m3Model * inNormal);
    outDto.tangent = normalize(m3Model * inTangent);

    outDto.ambientColor = globalUbo.ambientColor;
    outDto.viewPosition = globalUbo.viewPosition;

    gl_Position = globalUbo.projection * globalUbo.view * globalUbo.model * vec4(inPosition, 1.0);

    outMode = globalUbo.mode;
}
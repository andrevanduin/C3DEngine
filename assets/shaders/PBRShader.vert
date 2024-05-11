
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec3 inTangent;

layout(set = 0, binding = 0) uniform globalUniformObject 
{
	mat4 projection;
	mat4 view;
	mat4 lightSpace;
	vec4 ambientColor;
	vec3 viewPosition;
	int mode;
	int usePCF;
	float bias;
	vec2 padding;
} globalUbo;

// Push constants are only guaranteed to be a total of 128 bytes.
layout(push_constant) uniform PushConstants 
{
	mat4 model; // 64 bytes
} uPushConstants;

layout(location = 0) out int outMode;
layout(location = 1) out int usePCF;

// Data transfer object
layout(location = 2) out struct dto 
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
	
	// Fragment position in world space
	outDto.fragPosition = vec3(uPushConstants.model * vec4(inPosition, 1.0));

	// Calculate mat3 version of our model
	mat3 m3Model = mat3(uPushConstants.model);
	// Convert local normal to "world space"
	outDto.normal = normalize(m3Model * inNormal);
	outDto.tangent = normalize(m3Model * inTangent);
	outDto.ambient = globalUbo.ambientColor;
	outDto.viewPosition = globalUbo.viewPosition;
	
	gl_Position = globalUbo.projection * globalUbo.view * uPushConstants.model * vec4(inPosition, 1.0);

	outDto.lightSpaceFragPosition = (bias * globalUbo.lightSpace) * vec4(outDto.fragPosition, 1.0);

	outMode = globalUbo.mode;
	usePCF = globalUbo.usePCF;
	outDto.bias = globalUbo.bias;
}
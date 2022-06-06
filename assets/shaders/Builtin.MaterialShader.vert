
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec4 inTangent;

layout(set = 0, binding = 0) uniform globalUniformObject 
{
	mat4 projection;
	mat4 view;
	vec4 ambientColor;
	vec3 viewPosition;
	int mode;
} globalUbo;

// Push constants are only guaranteed to be a total of 128 bytes.
layout(push_constant) uniform pushConstants 
{
	mat4 model; // 64 bytes
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
	vec4 tangent;
} outDto;

void main()
{
	// Simply pass ambient color
	outDto.ambientColor = globalUbo.ambientColor;
	// Simply pass texture coordinates
	outDto.texCoord = inTexCoord;
	// Simply pass view position
	outDto.viewPosition = globalUbo.viewPosition;
	// Fragment position in world space
	outDto.fragPosition = vec3(uPushConstants.model * vec4(inPosition, 1.0));
	// Simply pass color
	outDto.color = inColor;

	// Calculate mat3 version of our model
	mat3 m3Model = mat3(uPushConstants.model);
	// Convert local normal to "world space"
	outDto.normal = m3Model * inNormal;
	outDto.tangent = vec4(normalize(m3Model * inTangent.xyz), inTangent.w);

	outMode = globalUbo.mode;

	gl_Position = globalUbo.projection * globalUbo.view * uPushConstants.model * vec4(inPosition, 1.0);
}
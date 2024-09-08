#version 450

layout(set = 1, binding = 0) uniform sampler2D colorMap;

layout(location = 1) in struct Dto
{
    vec2 texCoord;
} inDto;

layout(location = 0) out vec4 outColor;

void main()
{
    float alpha = texture(colorMap, inDto.texCoord).a;
    outColor = vec4(1.0, 1.0, 1.0, alpha);
    if (alpha < 0.5)
    {
        discard;
    }
}
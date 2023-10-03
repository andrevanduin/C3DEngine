#material file

version = 2
type = custom
name = falc_wreck
shader = Shader.Builtin.Material

[map]
name = diffuse
minifyFilter = linear
magnifyfilter = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = falc_wreck_low_DefaultMaterial_AlbedoTransparency
[/map]

[map]
name = specular
minifyFilter = linear
magnifyfilter = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = falc_wreck_low_DefaultMaterial_MetallicSmoothness
[/map]

[map]
name = normal
minifyFilter = linear
magnifyfilter = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = falc_wreck_low_DefaultMaterial_Normal
[/map]

[prop]
name = diffuseColor
type = vec4
value = 0.800000 0.800000 0.800000 1.000000
[/prop]

[prop]
name = padding
type = vec3
value = 0.0 0.0 0.0
[/prop]

[prop]
name = shininess
type = f32
value = 8.0
[/prop]
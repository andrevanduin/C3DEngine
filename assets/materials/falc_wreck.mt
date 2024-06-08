#material file

version = 2
type = pbr
name = falc_wreck

[map]
name = albedo
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = falc_wreck_low_DefaultMaterial_AlbedoTransparency
[/map]

[map]
name = combined
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = falc_wreck_low_DefaultMaterial_combined
[/map]

[map]
name = normal
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = falc_wreck_low_DefaultMaterial_Normal
[/map]

[prop]
name = shininess
type = f32
value = 10
[/prop]
[prop]
name = padding
type = vec3
value = 0.000000 0.000000 0.000000
[/prop]
[prop]
name = diffuseColor
type = vec4
value=0.588000 0.588000 0.588000 1.000000
[/prop]
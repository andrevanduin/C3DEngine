#material file

version = 2
type = pbr
name = fabric_c
shader = Shader.PBR

[map]
name = albedo
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = sponza_curtain_diff
[/map]

[map]
name = combined
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = sponza_curtain_combined
[/map]

[map]
name = normal
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = sponza_curtain_ddn
[/map]

[prop]
name = diffuseColor
type = vec4
value = 0.588 0.588 0.588 1
[/prop]
[prop]
name = padding
type = vec3
value = 0 0 0
[/prop]
[prop]
name = shininess
type = f32
value = 10
[/prop]

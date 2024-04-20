#material file

version = 2
type = pbr
name = vase
shader = Shader.PBR
[map]
name = albedo
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = vase_dif
[/map]
[map]
name = specular
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = vase_spec
[/map]
[map]
name = normal
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = vase_ddn
[/map]
[map]
name = normal
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = vase_ddn
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

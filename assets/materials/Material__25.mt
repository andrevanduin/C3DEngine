#material file

version = 2
type = pbr
name = Material__25
shader = Shader.PBR
[map]
name = albedo
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = lion
[/map]
[map]
name = specular
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = lion_spec
[/map]
[map]
name = roughness
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = lion_roughness
[/map]
[map]
name = normal
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = lion_ddn
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

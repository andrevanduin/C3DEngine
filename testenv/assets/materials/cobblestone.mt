#material file

version = 2
# types can be phong, pbr or custom
type = custom
name = cobblestone

[map]
name = albedo
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = cobblestone
[/map]

[map]
name = specular
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = cobblestone_specular
[/map]

[map]
name = normal
filterMin = linear
filterMag = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = cobblestone_normal
[/map]

[prop]
name = diffuseColor
type = vec4
value = 1.0 1.0 1.0 1.0
[/prop]

[prop]
name = shininess
type = f32
value = 15.0
[/prop]
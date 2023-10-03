#material file

version = 2
# types can be phong, pbr or custom
type = custom
name = cobblestone
# If custom, shader is required
shader = Shader.Builtin.Material

[map]
name = diffuse
minifyFilter = linear
magnifyfilter = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = cobblestone
[/map]

[map]
name = specular
minifyFilter = linear
magnifyfilter = linear
repeatU = repeat
repeatV = repeat
repeatW = repeat
textureName = cobblestone_specular
[/map]

[map]
name = normal
minifyFilter = linear
magnifyfilter = linear
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
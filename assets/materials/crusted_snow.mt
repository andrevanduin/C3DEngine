#material file

version = 2
type = pbr
name = crusted_snow

[map]
name=albedo
filterMin=linear
filterMag=linear
repeatU=repeat
repeatV=repeat
repeatW=repeat
textureName=crusted_snow_albedo
[/map]

[map]
name=normal
filterMin=linear
filterMag=linear
repeatV=repeat
repeatU=repeat
repeatW=repeat
textureName=crusted_snow_normal
[/map]

[map]
name=combined
filterMin=linear
filterMag=linear
repeatU=repeat
repeatV=repeat
repeatW=repeat
textureName=crusted_snow_combined
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
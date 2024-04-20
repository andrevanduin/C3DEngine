#material file

version = 2
type = pbr
name = wavy_sand

[map]
name=albedo
filterMin=linear
filterMag=linear
repeatU=repeat
repeatV=repeat
repeatW=repeat
textureName=wavy_sand_albedo
[/map]

[map]
name=metallic
filterMin=linear
filterMag=linear
repeatU=repeat
repeatV=repeat
repeatW=repeat
textureName=wavy_sand_metallic
[/map]

[map]
name=roughness
filterMin=linear
filterMag=linear
repeatU=repeat
repeatV=repeat
repeatW=repeat
textureName=wavy_sand_roughness
[/map]

[map]
name=normal
filterMin=linear
filterMag=linear
repeatV=repeat
repeatU=repeat
repeatW=repeat
textureName=wavy_sand_normal
[/map]

[map]
name=ao
filterMin=linear
filterMag=linear
repeatV=repeat
repeatU=repeat
repeatW=repeat
textureName=wavy_sand_ao
[/map]

[prop]
name=diffuseColor
type=vec4
value=0.588000 0.588000 0.588000 1.000000
[/prop]

# The material shader requires this padding to be in place.
[prop]
name=padding
type=vec3
value=0.000000 0.000000 0.000000
[/prop]

[prop]
name=shininess
type=f32
value=10.0
[/prop]
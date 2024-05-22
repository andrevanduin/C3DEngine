#material file

version = 2
type = pbr
name = river_rock

[map]
name=albedo
filterMin=linear
filterMag=linear
repeatU=repeat
repeatV=repeat
repeatW=repeat
textureName=river_rock_albedo
[/map]

[map]
name=normal
filterMin=linear
filterMag=linear
repeatV=repeat
repeatU=repeat
repeatW=repeat
textureName=river_rock_normal
[/map]

[map]
name=combined
filterMin=linear
filterMag=linear
repeatV=repeat
repeatU=repeat
repeatW=repeat
textureName=river_rock_combined
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
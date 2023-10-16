#!/bin/bash
pushd "assets/shaders"
shopt -s nullglob
for file in *.{frag,vert}; do
    echo "Compiling $file -> $file.spv" &&
    ~/vulkan/1.3.261.1/x86_64/bin/glslc $file -o $file.spv
done
popd
@echo OFF

pushd "assets/shaders"
for %%i in (*.frag *.vert *.comp) do (
    echo "Compiling %%i -> %%i.spv"
    call "%VULKAN_SDK%\Bin\glslc.exe" %%i -o %%i.spv
)
popd
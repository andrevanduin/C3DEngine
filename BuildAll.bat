@echo OFF

pushd "build\testenv\lib"
del *.pdb
del TestEnvLib.dll
popd

REM Take the first argument that was provided by the user
set cfg=%~1

REM Check if no argument was provided and if not set to default (Debug)
if "%cfg%" == "" (
    set cfg=Debug
)

cmake --build c:/Users/Cesar/Desktop/dev/C3DEngine/build --config %cfg% -- -j 8
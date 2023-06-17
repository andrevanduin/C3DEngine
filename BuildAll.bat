@echo OFF

pushd "build\testenv\lib"
del *.pdb
del TestEnvLib.dll
popd

cmake --build c:/Users/Cesar/Desktop/dev/C3DEngine/build --config Debug
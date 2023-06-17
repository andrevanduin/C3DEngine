@echo OFF

REM Set the VERSION variable to the output of the VersionGen binary
@SET VERSION =
FOR /F %%I IN ('VersionGen') DO @SET "VERSION=%%I"

REM Use the generated version as input for regenerating cmake so the PDB name is changed every time
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Debug -DRANDOM_PDB_VERSION:STRING=%VERSION% -SC:/Users/Cesar/Desktop/dev/C3DEngine -Bc:/Users/Cesar/Desktop/dev/C3DEngine/build -G Ninja

REM Actually build our library code
cmake --build c:/Users/Cesar/Desktop/dev/C3DEngine/build --config Debug --target TestEnvLib --
@echo off
REM =============================================================================
REM Compile GLSL shaders to SPIR-V using glslc (from the Vulkan SDK)
REM Run from the shaders/ directory, or from project root.
REM Output goes to the build directory alongside the executable.
REM =============================================================================

set SHADER_DIR=%~dp0
set OUT_DIR=%SHADER_DIR%..\..\..\..\build\Release\shaders

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

echo Compiling sprite.vert...
glslc "%SHADER_DIR%sprite.vert" -o "%OUT_DIR%\sprite.vert.spv"
if %errorlevel% neq 0 ( echo FAILED & exit /b 1 )

echo Compiling sprite.frag...
glslc "%SHADER_DIR%sprite.frag" -o "%OUT_DIR%\sprite.frag.spv"
if %errorlevel% neq 0 ( echo FAILED & exit /b 1 )

echo All shaders compiled successfully to %OUT_DIR%

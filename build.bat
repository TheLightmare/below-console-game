@echo off
echo Building Console Game for Windows...

if not exist build mkdir build
cd build

cmake ..
if errorlevel 1 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

cmake --build . --config Release
if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo Copying assets to build\Release...
xcopy /E /I /Y "..\assets" "Release\assets" >nul
xcopy /Y "..\keybindings.cfg" "Release\" >nul

echo.
echo Build successful!
echo Executable: build\Release\game.exe
echo.
pause

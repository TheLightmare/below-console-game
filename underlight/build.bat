@echo off
cmake -B build -DBUILD_VULKAN_RENDERER=ON && cmake --build build --config Release
pause

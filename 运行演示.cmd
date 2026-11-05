@echo off
cd /d "%~dp0"
if not exist build\lab.exe (
  powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1
  if errorlevel 1 goto failed
)
build\lab.exe
pause
exit /b
:failed
echo Build failed. Please check GCC PATH.
pause
exit /b 1

@echo off
REM Compila La Pauta SL Channel y genera el instalador.
REM Requisitos: Visual Studio 2022 (C++), CMake e Inno Setup 6.

cmake -B build -G "Visual Studio 17 2022" -A x64 || goto :error
cmake --build build --config Release || goto :error

"%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" installer.iss || goto :error

echo.
echo Listo: installer_output\LaPautaSLChannel-Setup-1.0.0.exe
exit /b 0

:error
echo.
echo Algo fallo. Revisa el mensaje de arriba.
exit /b 1

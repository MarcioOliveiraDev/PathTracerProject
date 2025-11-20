@echo off
echo Compilando Path Tracer...

g++ -O3 -march=native -fopenmp -std=c++17 ^
    src/main.cpp src/stb_impl.cpp ^
    -I include ^
    -o pathtracer.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Compilacao concluida com sucesso!
    echo Execute: pathtracer.exe
) else (
    echo.
    echo Erro na compilacao!
)

pause

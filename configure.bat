@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat"
where ninja 2>nul
if %errorlevel% neq 0 (
    echo Ninja not found, using NMake Makefiles
    cmake -B build -S . -G "NMake Makefiles" -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64 -DCMAKE_BUILD_TYPE=Release
) else (
    echo Using Ninja
    cmake -B build -S . -G Ninja -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64 -DCMAKE_BUILD_TYPE=Release
)

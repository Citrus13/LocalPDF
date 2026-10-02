@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat"
echo --- cl.exe check ---
where cl
echo --- cl version ---
cl 2>&1
echo --- cmake version ---
cmake --version
echo --- MSVC dir ---
echo %VCToolsInstallDir%

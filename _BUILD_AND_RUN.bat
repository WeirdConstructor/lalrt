cd build
IF NOT DEFINED VCINSTALLDIR call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64_x86
C:\local\cmake\bin\cmake.exe --build . --target LALRT
if not %ERRORLEVEL% == 1 (
    cd ..
    .\LALRT.exe
) else (
    cd ..
)

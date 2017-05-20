cd build
IF NOT DEFINED VCINSTALLDIR call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64_x86
rem ..\..\..\tools\cmake\bin\cmake.exe -DCMAKE_GENERATOR_PLATFORM=x64 ..
..\..\..\tools\cmake\bin\cmake.exe -T v140_xp -DCMAKE_GENERATOR_TOOLSET=v140_xp ..
..\..\..\tools\cmake\bin\cmake.exe --build .
if not %ERRORLEVEL% == 1 (
    ..\..\..\tools\cmake\bin\ctest -C Debug --output-on-failure .
)
if not %ERRORLEVEL% == 1 (
    ..\lalrt ..\tests\lua\mp_tests.lua
)
cd ..

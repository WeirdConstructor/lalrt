cd build
IF NOT DEFINED VCINSTALLDIR call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64_x86
rem %BZVC_CMAKE%\cmake.exe -T v140_xp -DCMAKE_GENERATOR_TOOLSET=v140_xp ..
rem C:\local\cmake\bin\cmake.exe -DCMAKE_GENERATOR_PLATFORM=x64 ..
C:\local\cmake\bin\cmake.exe -T v140_xp -DCMAKE_GENERATOR_TOOLSET=v140_xp ..
C:\local\cmake\bin\cmake.exe --build . --config Release
set LOCAL_ERR=%ERRORLEVEL%
cd ..
cmd /c exit %LOCAL_ERR%

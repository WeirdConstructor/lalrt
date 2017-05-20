cd build
IF NOT DEFINED VCINSTALLDIR call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
C:\local\cmake\bin\cmake.exe -G "Visual Studio 14" -DWITH_POSTGRESQL=OFF -DWITH_FIREBIRD=OFF -DWITH_DB2=OFF -DWITH_MYSQL=OFF -DCMAKE_GENERATOR_PLATFORM=x64 ..
C:\local\cmake\bin\cmake.exe --build .
set LOCAL_ERR=%ERRORLEVEL%
cd ..
cmd /c exit %LOCAL_ERR%

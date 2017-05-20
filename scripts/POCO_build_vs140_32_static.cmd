@echo off

set INCLUDE=%INCLUDE%;%~dp0\..\OpenSSL-Win32\include\
set LIB=%LIB%;%~dp0\..\OpenSSL-Win32\lib\VC

call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" amd64_x86
rem buildwin 140 build static_md both x64 samples
CALL buildwin 140 build static_md release Win32 samples

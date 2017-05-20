@echo off
call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" amd64_x86
buildwin 140 clean static_md both Win32 samples

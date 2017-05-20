CALL "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/vcvarsall.bat" amd64_x86
rem .\bootstrap.bat
rem .\b2 --stagedir=stage_static     --build-dir=build_static variant=release define=BOOST_USE_WINAPI_VERSION=0x0501 link=static architecture=x86 address-model=32 toolset=msvc-14.0 threading=multi runtime-link=shared
.\b2 --clean --stagedir=stage_static --build-dir=build_static variant=release define=BOOST_USE_WINAPI_VERSION=0x0501 link=static architecture=x86 address-model=32 toolset=msvc-14.0 threading=multi runtime-link=shared
.\b2 --stagedir=stage_static         --build-dir=build_static variant=release define=BOOST_USE_WINAPI_VERSION=0x0501 link=static architecture=x86 address-model=32 toolset=msvc-14.0 threading=multi runtime-link=shared

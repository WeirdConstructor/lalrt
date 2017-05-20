@mkdir dist
@xcopy /Y /E dist_src\*     dist\
@copy lalrt.exe          dist\lalrt.exe
@xcopy /Y /E lalrtlib       dist\lalrtlib\
@xcopy /Y /E dist_src\*     .\

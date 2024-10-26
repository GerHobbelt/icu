@echo off
:: Build the icuuc and icuin dlls to forward to icu.dll

set original_directory="%cd%"
set arch=%1
set out_dir=

IF %arch%==x64 set out_dir="..\..\bin64"
IF %arch%==x86 set out_dir="..\..\bin"

IF [%out_dir%]==[] echo Invalid architecture: %arch% (must be either x64 or x86) && EXIT /b 1

echo. Got Arch: %arch%
echo. Got Output directory: %out_dir%

cd %~dp0
cd ..\..\icu\icu4c\source\common
:: Not sure if we can build both architectures like this at once.
call:compile_dll icuuc
call:compile_dll icuin

cd %original_directory%

EXIT /b %ERRORLEVEL%

:compile_dll
echo. Got Library: %~1

:: Cleanup since last run
del cmemory.obj %~1.dll %~1.lib %~1.exp ucln_cmn.lib ucln_cmn.exp ucln_cmn.obj umutex.obj utrace.obj
:: Generate a .lib with the required forwarding rules
lib /MACHINE:%arch% /nologo /def:..\common-and-i18n\%~1.def
:: Generate a .dll with just the .exp generated above, no code - just forward
link %1.exp /NOENTRY /nologo /out:%~1.dll /MACHINE:%arch% /DLL /release /SUBSYSTEM:CONSOLE /guard:cf
move %~1.dll %out_dir%\%~1.dll
:goto :eof
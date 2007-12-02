@echo off

rem Sets up the Allegro package for building with the specified compiler,
rem and if possible converting text files from LF to CR/LF format.

rem Test if there are too many args.
if [%4] == []        goto arg3
goto help

:arg3
rem Test if third arg is ok.
if [%3] == [--crlf]        goto arg2
if [%3] == [--quick]       goto arg2
if [%3] == [--msvcpaths]   goto arg2
if [%3] == [--nomsvcpaths] goto arg2
if [%3] == []              goto arg2
goto help

:arg2
rem Test if second arg is ok.
if [%2] == [--crlf]        goto arg1
if [%2] == [--quick]       goto arg1
if [%2] == [--msvcpaths]   goto arg1
if [%2] == [--nomsvcpaths] goto arg1
if [%2] == []              goto arg1
goto help

:arg1
rem Test if first arg is ok.
if [%1] == [bcc32]   goto head
if [%1] == [djgpp]   goto head
if [%1] == [mingw]   goto head
if [%1] == [mingw32] goto head
if [%1] == [dmc]     goto head
if [%1] == [msvc]    goto head
if [%1] == [msvc6]   goto head
if [%1] == [msvc7]   goto head
if [%1] == [msvc8]   goto head
if [%1] == [icl]     goto head
if [%1] == [watcom]  goto head
goto help


:head
rem Generate header of makefile and alplatf.h,
rem then go to platform specific function.
echo # generated by fix.bat > makefile
echo /* generated by fix.bat */ > include\allegro\platform\alplatf.h

if [%1] == [bcc32]   goto bcc32
if [%1] == [djgpp]   goto djgpp
if [%1] == [mingw]   goto mingw
if [%1] == [mingw32] goto mingw
if [%1] == [dmc]     goto dmc
if [%1] == [msvc]    goto msvc
if [%1] == [msvc6]   goto msvc6
if [%1] == [msvc7]   goto msvc7
if [%1] == [msvc8]   goto msvc8
if [%1] == [icl]     goto icl
if [%1] == [watcom]  goto watcom

echo fix.bat internal error: not reached
goto help

:bcc32
echo Configuring Allegro for Windows/BCC32...
echo MAKEFILE_INC = makefile.bcc >> makefile
echo #define ALLEGRO_BCC32 >> include\allegro\platform\alplatf.h
goto tail

:djgpp
echo Configuring Allegro for DOS/djgpp...
echo MAKEFILE_INC = makefile.dj >> makefile
echo #define ALLEGRO_DJGPP >> include\allegro\platform\alplatf.h
goto tail

:mingw
echo Configuring Allegro for Windows/MinGW...
echo MAKEFILE_INC = makefile.mgw >> makefile
echo #define ALLEGRO_MINGW32 >> include\allegro\platform\alplatf.h
goto tail

:dmc
echo Configuring Allegro for Windows/DMC...
echo MAKEFILE_INC = makefile.dmc >> makefile
echo #define ALLEGRO_DMC >> include\allegro\platform\alplatf.h
goto tail

:icl
echo Configuring Allegro for Windows/ICL...
echo COMPILER_ICL = 1 >> makefile
goto msvccommon

:msvc8
echo Configuring Allegro for Windows/MSVC8...
echo COMPILER_MSVC8 = 1 >> makefile
goto msvccommon

:msvc7
echo Configuring Allegro for Windows/MSVC7...
echo COMPILER_MSVC7 = 1 >> makefile
goto msvccommon

:msvc
echo.
echo Notice: Because no version was specified, MSVC 6 has been chosen. 
echo.
echo If you are using a newer version, you should use 'msvc7' or 'msvc8' instead.
echo msvc7 should be used for MSVC .NET or MSVC .NET 2003
echo msvc8 should be used for MSVC .NET 2005
echo.

:msvc6
echo Configuring Allegro for Windows/MSVC6...
goto msvccommon

:msvccommon
echo MAKEFILE_INC = makefile.vc >> makefile
echo #define ALLEGRO_MSVC >> include\allegro\platform\alplatf.h
if "%MSVCDir%" == "" set MSVCDir=%VCINSTALLDIR%
if "%MSVCDir%" == "" echo Your MSVCDir environment variable is not set!

REM msvc6 does not need this, msvc is fallback so we should do it anyway
if [%1] == [msvc6]         goto skipconvert
if [%2] == [--nomsvcpaths] goto skipconvert
if [%3] == [--nomsvcpaths] goto skipconvert

echo Converting MSVCDir path...
cl /nologo /w misc/msvchelp.c >NUL
msvchelp MSVCDir
del msvchelp.exe
del msvchelp.obj
echo include makefile.helper >> makefile
goto tail

:skipconvert
REM Don't put space before >> !
echo MSVCDir = %MSVCDir%>> makefile
goto tail

:watcom
echo Configuring Allegro for DOS/Watcom...
echo MAKEFILE_INC = makefile.wat >> makefile
echo #define ALLEGRO_WATCOM >> include\allegro\platform\alplatf.h
goto tail

:help
echo.
echo Usage: fix platform [--crlf] [--nomsvcpaths]
echo.
echo Where platform is one of:
echo     bcc32, djgpp, mingw, dmc, msvc6, msvc7, msvc8, icl, or watcom.
echo.
echo The --crlf parameter is used to turn on LF to CR/LF conversion.
echo The --nomsvcpaths parameter is used to turn off special MS Visual C++
echo path handling.
echo.
goto end

:tail
rem Generate last line of makefile and optionally convert CR/LF.
echo include makefile.all >> makefile

if [%2] == [--crlf] goto crlf
if [%3] == [--crlf] goto crlf

goto done

:crlf
echo Converting Allegro files to DOS CR/LF format...
utod .../*.bat .../*.sh .../*.c *.cfg .../*.h .../*.inc .../*.rc
utod .../*.rh .../*.inl .../*.s .../*.txt .../*._tx makefile.*

:done
echo Done!

:end

@echo off
cls
REM		Build Script

REM Set Compiler Settings Here

set CPP=c++
set GPP=g++

set OUTPUT=test.exe
set DEBUGMODE=0
set CLI_INTERFACE=1
set EXTRA_COMPONENTS=0
set RUN=0

set LINK_ONLY=0
set ASYNCBUILD=1

set ONLY_BUILD_PARAMETERS=0
set REBUILD_APPLICATION=1

set COMPILER_FLAGS=-std=c++20
set ADDITIONAL_LIBRARIES=-static -static-libstdc++ -lpthread -lpng -lz -luser32 -lopengl32 -lgdiplus -lShlwapi -ldwmapi -lstdc++fs -lcrypto -lcrypt32 -lws2_32 -lgdi32 -lsetupapi -lwinmm
set ADDITIONAL_LIBDIRS=-Llibraries\libopenssl\lib -Llibraries\libpng\lib -Llibraries\libz\lib
set ADDITIONAL_INCLUDEDIRS=-Iinclude -Ilibraries\libopenssl\include -Ilibraries\librapidjson\include -Ilibraries\libpng\include -Ilibraries\libz\include -Ilibraries\extra -I.


:: -- Build Script Start --

echo --------------------------------------------------
echo -  Initializing EvoAlgo 3 Build Toolchain v1.2   -
echo --------------------------------------------------

del /Q /R %OUTPUT% 2>nul

setlocal enabledelayedexpansion

if %EXTRA_COMPONENTS% GTR 0 (
	set COMPILER_FLAGS=%COMPILER_FLAGS% -DWEXTRA_COMPONENTS
)

if %LINK_ONLY% GTR 0 (
	goto linker
)

if %ONLY_BUILD_PARAMETERS% GTR 0 (
	goto params
)

if %REBUILD_APPLICATION% GTR 0 (
	del /S /Q ".objs64\*.o"
)


if not exist .objs64 (
	echo Creating Object Directory Structure...
	mkdir .objs64
)

if %DEBUGMODE% GTR 0 (
	set DEBUG_INFO=-ggdb -g
) else (
	set DEBUG_INFO=-s
	set COMPILER_FLAGS=%COMPILER_FLAGS% -O2
)

echo Building CPP Files...
for %%F in (src\*.cpp) do (
	if not exist .objs64\%%~nF.o (
		echo Building %%~nF.o
		if %ASYNCBUILD% GTR 0 (
			start /B "%%~nF.o" %CPP% %DEBUG_INFO% %ADDITIONAL_INCLUDEDIRS% %COMPILER_FLAGS% -c %%F -o .objs64\%%~nF.o
		) else (
			%CPP% %DEBUG_INFO% %ADDITIONAL_INCLUDEDIRS% %COMPILER_FLAGS% -c %%F -o .objs64\%%~nF.o
		)
	)
)

REM  Jump to loop
goto loop

REM Build Only Parameters
:params
del /S /Q ".objs64\parameters.o"
echo Rebuilding The Parameters...
start /B "parameters.o" %CPP% %DEBUG_INFO% %ADDITIONAL_INCLUDEDIRS% %COMPILER_FLAGS% -c src\parameters.cpp -o .objs64\parameters.o


REM Wait for building process to finish
:loop
for /f %%G in ('tasklist ^| find /c "%CPP%"') do ( set count=%%G )
if %count%==0 (
	goto linker
) else (
	timeout /t 2 /nobreak>nul
	goto loop
)

:linker

set "files="
for /f "delims=" %%A in ('dir /b /a-d ".objs64\*.o" ') do set "files=!files! .objs64\%%A"
if %EXTRA_COMPONENTS% GTR 0 (
	:: do not use adw.o unless you know what you are doing
	for /f "delims=" %%A in ('dir /b /a-d "libraries\extra\*.o" ') do set "files=!files! libraries\extra\%%A"
)

:link
echo Linking Executable...

if %CLI_INTERFACE% GTR 0 (
	set MWINDOWS=
) else (
	set MWINDOWS=-mwindows
)

%GPP% -o %OUTPUT% %files% %DEBUG_INFO% %ADDITIONAL_LIBDIRS% %ADDITIONAL_LIBRARIES% %MWINDOWS%

:finish
if exist .\%OUTPUT% (
	echo -------------------------------
	echo -  EvoAlgo Build Completed!   -
	echo -------------------------------
	if %RUN% GTR 0 (
		start /B /WAIT .\%OUTPUT%
	)
) else (
	echo ----------------------------
	echo -  EvoAlgo Build FAILED!   -
	echo -  See Log For Details     -
	echo ----------------------------
)

REM pause>nul

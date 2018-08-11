@echo off
@echo.
@echo. Build_Helper [OutputDir] [BuildCmd]
@echo. Arch: X64 only
@echo. BuildTarget: DEBUG only
@echo. BuildCmd: NORMAL or CLEAN or REBUILD
@echo. Usage: Build_Helper E:\Build NORMAL
@echo.

set __OUTPUTDIR=%1
set __BUILDCMD=%2

rem Remove the double quotes.
set __OUTPUTDIR=%__OUTPUTDIR:"=%

set __ARCH=
set __TARGETFILE=
set __TARGETDIR=
set __RESULTFILE=

rem DO NOT reset variables in if statement.
rem if <cond> (
rem    set VAR=TEST
rem    set VAR2=%VAR%
rem )
rem results VAR2=<empty>.

set __BUILDTARGET=DEBUG
set __ARCH=X64
set __TOOLSETTAG=VS2015
set __PACKAGEDSC=DuetPkg/DuetPkgX64.dsc
set __TARGETDIR=Build\DuetPkgX64\%__BUILDTARGET%_VS2015\X64
set __TARGETFILE=Build\DuetPkgX64\%__BUILDTARGET%_VS2015\X64\OSLDR.efi
set __RESULTFILE=%__OUTPUTDIR%\BOOTx64.efi

rem To recompile our source, we must delete the "%__TARGETDIR%\OSLDR".
if exist "%__RESULTFILE%" del "%__RESULTFILE%"
if exist "%__TARGETDIR%\OSLDR" rmdir /s /q "%__TARGETDIR%\OSLDR"

call edksetup.bat --nt32

if "%__BUILDCMD%" == "CLEAN" (
    rmdir /s /q %__TARGETDIR%
) else if "%__BUILDCMD%" == "REBUILD" (
    rmdir /s /q %__TARGETDIR%
    call build -a %__ARCH% -b %__BUILDTARGET% -t %__TOOLSETTAG% -p %__PACKAGEDSC%
	@echo. copy "%__TARGETFILE%" to "%__RESULTFILE%"
    copy "%__TARGETFILE%" "%__RESULTFILE%" /y
) else (
    rem treat as normal build
    call build -a %__ARCH% -b %__BUILDTARGET% -t %__TOOLSETTAG% -p %__PACKAGEDSC%
	@echo. copy "%__TARGETFILE%" to "%__RESULTFILE%"
    copy "%__TARGETFILE%" "%__RESULTFILE%" /y
)

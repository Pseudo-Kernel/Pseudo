
@echo off

set __ERROR__=0

if "%~1" == "" set __ERROR__=1
if "%~2" == "" set __ERROR__=1
if "%~3" == "" set __ERROR__=1

if %__ERROR__% == 1 (
  @echo ERROR: usage: %0 ^<BootImagePath^> ^<AssignDriveLetterTemp^> ^<FilesystemContentsPath^>
  goto :EOF
)

rem goto :EOF
rem exit

@echo Preparing...

set SCRIPT_INIT=%cd%\image_init.dps
set SCRIPT_CLEANUP=%cd%\image_detach.dps

set BOOT_IMAGE_PATH=%~1
set ASSIGN_DRIVE_LETTER=%~2
set FILESYSTEM_CONTENTS_PATH=%~3

@echo Current Path             = %cd%
@echo Boot Image Path          = %BOOT_IMAGE_PATH%
@echo Assign Drive Letter      = %ASSIGN_DRIVE_LETTER%
@echo Filesystem Contents Path = %FILESYSTEM_CONTENTS_PATH%


if exist "%BOOT_IMAGE_PATH%" (
  @echo Deleting the previous image...
  del "%BOOT_IMAGE_PATH%"
)

rem
rem Create the diskpart script which creates FAT32 boot image.
rem

type nul                                                            > %SCRIPT_INIT%
echo create vdisk file="%BOOT_IMAGE_PATH%" maximum=132 type=fixed  >> %SCRIPT_INIT%
echo select vdisk file="%BOOT_IMAGE_PATH%"                         >> %SCRIPT_INIT%
echo attach vdisk                                                  >> %SCRIPT_INIT%
echo convert gpt                                                   >> %SCRIPT_INIT%
echo create partition efi                                          >> %SCRIPT_INIT%
echo format fs=fat32 quick                                         >> %SCRIPT_INIT%
echo assign letter=%ASSIGN_DRIVE_LETTER%                           >> %SCRIPT_INIT%
echo rescan                                                        >> %SCRIPT_INIT%
rem echo set id=c12a7328-f81f-11d2-ba4b-00a0c93ec93b        >> %SCRIPT_INIT%
echo exit                                                          >> %SCRIPT_INIT%

rem Create the diskpart script which detaches our boot image from volume.

type nul                                                            > %SCRIPT_CLEANUP%
echo select vdisk file="%BOOT_IMAGE_PATH%"                         >> %SCRIPT_CLEANUP%
echo select volume=%ASSIGN_DRIVE_LETTER%                           >> %SCRIPT_CLEANUP%
echo remove letter=%ASSIGN_DRIVE_LETTER%                           >> %SCRIPT_CLEANUP%
echo detach vdisk                                                  >> %SCRIPT_CLEANUP%
echo exit                                                          >> %SCRIPT_CLEANUP%



@echo Creating the FAT32 boot image...
diskpart /s %SCRIPT_INIT%

@echo Copying contents of directory...
@echo Execute: xcopy %FILESYSTEM_CONTENTS_PATH%\* %ASSIGN_DRIVE_LETTER%: /e /f /h /r /y
xcopy %FILESYSTEM_CONTENTS_PATH%\* %ASSIGN_DRIVE_LETTER%: /e /f /h /r /y

@echo Detaching...
diskpart /s %SCRIPT_CLEANUP%


if exist %SCRIPT_INIT% del %SCRIPT_INIT%
if exist %SCRIPT_CLEANUP% del %SCRIPT_CLEANUP%



param (
    [Parameter(Mandatory = $true)] [string]$BootImagePath,
    [Parameter(Mandatory = $true)] [string]$FilesystemContentsPath,
    [string]$AssignDriveLetterTemporary = "",
    [bool]$AssertElevated = $false
)

<#
if (!$AssertElevated) {
    $CommandLine = [string]::Format('-File "{0}" -{1} "{2}" -{3} "{4}" -{5} "{6}" -{7} "{8}"',
    $MyInvocation.PSCommandPath
        $MyInvocation.MyCommand.Path, 
        "BootImagePath", $BootImagePath, 
        "FilesystemContentsPath", $FilesystemContentsPath, 
        "AssignDriveLetterTemporary", $AssignDriveLetterTemporary, 
        "AssertElevated", $true);

    $ElevatedProcess = Start-Process -FilePath PowerShell.exe -Verb Runas
        -ArgumentList $CommandLine -Wait
    exit $ElevatedProcess.ExitCode
}
#>

if ([string]::IsNullOrWhiteSpace($AssignDriveLetterTemporary) -or 
    [string]::IsNullOrEmpty($AssignDriveLetterTemporary)) {
    "DriveLetter not specified, detecting... "

    # Automatically detect the drive letter which is free to use.
    [string]$FreeDriveLetterList = ""
    [int][char]'E'..[int][char]'W' | %{ $FreeDriveLetterList += [char]$_ }

    $FreeDriveLetterList = $FreeDriveLetterList.ToUpper()

    Get-Partition | ForEach-Object {
        $FreeDriveLetterList = $FreeDriveLetterList.Replace(([string]$_.DriveLetter).ToUpper(), "")
    }

    "OK."
    $AssignDriveLetterTemporary = $FreeDriveLetterList[$FreeDriveLetterList.Length - 1]
}

@"
Current Path             = $PWD
Boot Image Path          = $BootImagePath
Assign Drive Letter      = $AssignDriveLetterTemporary
Filesystem Contents Path = $FilesystemContentsPath
"@

# Create the diskpart script which creates FAT32 boot image.

# {0} = BootImagePath, {1} = AssignDriveLetterTemp
$ScriptCreateImage = @"
	create vdisk file="$BootImagePath" maximum=132 type=fixed
	select vdisk file="$BootImagePath"
	attach vdisk
	convert gpt
	create partition efi
	format fs=fat32 quick
	assign letter=$AssignDriveLetterTemporary
	rescan
"@
#	rem echo set id=c12a7328-f81f-11d2-ba4b-00a0c93ec93b

# {0} = BootImagePath, {1} = AssignDriveLetterTemp
$ScriptDetachImage = @"
	select vdisk file="$BootImagePath"
	select volume=$AssignDriveLetterTemporary
	remove letter=$AssignDriveLetterTemporary
	detach vdisk
"@

$ScriptPath_CreateImage = [System.IO.Path]::Combine($PWD, "create_image.diskpart")
$ScriptPath_DetachImage = [System.IO.Path]::Combine($PWD, "detach_image.diskpart")

# Create the FAT32 filesystem image for our OS.
# NOTE : it is better to use diskpart /s <filename> rather than using stdin directly
#        because of credential problem. (example: failed to write UAC-elevated process' stdin)
$Process = Start-Process -FilePath "diskpart" -ArgumentList ("/s", $ScriptPath_CreateImage) -Wait

# Copy our boot files to filesystem.
Copy-Item -Path $FilesystemContentsPath -Destination ($AssignDriveLetterTemporary + ":\") -Recurse -Verbose

# Detach the image from partition manager.
$Process = Start-Process -FilePath "diskpart" -ArgumentList ("/s", $ScriptPath_DetachImage) -Wait

# Clean up
Remove-Item -Path $ScriptPath_CreateImage -ErrorAction Ignore
Remove-Item -Path $ScriptPath_DetachImage -ErrorAction Ignore

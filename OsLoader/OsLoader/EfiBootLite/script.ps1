
# Win32.Kernel32 API Definition
$WinAPIDefinition = @'

[DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
public static extern bool MoveFile(string ExistingFileName, string NewFileName);
'@

Add-Type -MemberDefinition $WinAPIDefinition -Name 'Kernel32' -Namespace 'Win32' -PassThru


Get-ChildItem -Recurse -Force | foreach {
    "Renaming " + $_.FullName + " to lowercase"
    [Win32.Kernel32]::MoveFile($_.FullName, $_.FullName.ToLower()) | Out-Null}

<#
Get-ChildItem -Recurse -Force | foreach {
    Rename-Item $_.FullName -NewName $_.Name.ToLower() -ErrorAction Ignore
}
#>

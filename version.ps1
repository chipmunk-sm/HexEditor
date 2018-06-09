

param(
    [string] $platformId = "x64"
)

echo ""
echo "******************************************************"
echo "*** Create version info ***"
echo "******************************************************"
echo ""

if(!$platformId)
{
    Throw "platformId not set. Abort"
}

echo "`$platformId: $platformId"


$directory = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($PSScriptRoot)
echo "`$directory: $directory"

echo ""
echo "******************************************************"
echo "*** Parse GIT attributes..."
echo ""


$revision = "HEAD" 
$LastComm = "Test build"
$Build = $env:appveyor_build_number

$FullCommit = git -C $directory rev-parse $revision 2>$null
if (!$FullCommit) {
	Throw "Failed on get commit for $revision revision"
}

$LastTag = git -C $directory describe --tags --first-parent --match "*" $revision 2>$null
echo "TAG <$LastTag>"
if (!$LastTag) 
{
	$LastTag = "0.0.0.0"
	Write-Host "Failed on get tag for revision $revision - defaulting to $LastTag"
}
else
{
    #git tag -l --format='%(contents)' <tag name>
    $LastComm = git -C $directory tag -l --format='%(contents)' $LastTag
    if(!$LastComm)
    {
        $LastComm = "Auto-build"
    }

    $LastComm = $LastComm.Replace("`n"," ")
    $LastComm = $LastComm.Replace("`r"," ")
    $LastComm = $LastComm.Replace(" ", "_")
}

echo "Message: <$LastComm>" 


if ($LastTag -match "^[^\d]?[-|\.]?(\d+)\.(\d+)\.(\d+)\.?(\d+)?")
{
	$Major = [System.Convert]::ToUInt16($Matches[1])
	$Minor = [System.Convert]::ToUInt16($Matches[2])
	$Patch = [System.Convert]::ToUInt16($Matches[3])
	if(!$Build)
    {
		$Build = [System.Convert]::ToUInt16($Matches[4])
	    if(!$Build)
        {
            $Build = 0
        }
	}
    #echo "[$LastTag] => [$Major.$Minor.$Patch.$Build]"
}
else
{
	Throw "Failed on detected tag. [$LastTag] - invalid tag"
}

echo "[$LastTag] => [$Major.$Minor.$Patch.$Build]"

$ShortCommit = $FullCommit.SubString(0, 7)

$Year = (Get-Date).Year
$Month = (Get-Date).Month
$Day = (Get-Date).Day
$Hour = (Get-Date).Hour
$Minute = (Get-Date).Minute
$Second = (Get-Date).Second
$Millisecond = (Get-Date).Millisecond

echo ""
echo "******************************************************"
echo "*** Start update ver.h..."
echo ""

$output = "ver.h"

$new_output_contents = @"

#define VER_MAJOR $Major
#define VER_MINOR $Minor
#define VER_PATCH $Patch
#define VER_BUILD $Build
#define VER_SHORTCOMMIT $ShortCommit
#define VER_YEAR $Year
#define VER_MONTH $Month
#define VER_DAY $Day
#define VER_HOUR $Hour
#define VER_MINUTE $Minute
#define VER_SECOND $Second
#define VER_MILLISECOND $Millisecond
#define VER_FULLCOMMIT $FullCommit
#define S_VER_MAJOR "$Major"
#define S_VER_MINOR "$Minor"
#define S_VER_PATCH "$Patch"
#define S_VER_BUILD "$Build"
#define S_VER_SHORTCOMMIT "$ShortCommit"
#define S_VER_YEAR "$Year"
#define S_VER_MONTH "$Month"
#define S_VER_DAY "$Day"
#define S_VER_HOUR "$Hour"
#define S_VER_MINUTE "$Minute"
#define S_VER_SECOND "$Second"
#define S_VER_MILLISECOND "$Millisecond"
#define FVER_NAME "$LastComm"
#define FVER1 VER_MAJOR
#define FVER2 VER_MINOR
#define FVER3 VER_BUILD
#define FVER4 0

"@

$current_output_contents = Get-Content (Join-Path $directory $output) -Raw -Encoding Ascii -ErrorAction Ignore
if ([string]::Compare($new_output_contents, $current_output_contents, $False) -ne 0) {
	Write-Host "Updating $(Join-Path $directory $output)"
	[System.IO.File]::WriteAllText((Join-Path $directory $output), $new_output_contents)
}

echo ""
echo "******************************************************"
echo "*** Start update include.wxi..."
echo ""

# Upgrade code HAS to be the same for all updates!!!
$UpgradeCode = "ED1DA208-1973-445F-BADD-80B2E7CFE22A"
$ProductCode = [guid]::NewGuid().ToString().ToUpper()
$PackageCode = [guid]::NewGuid().ToString().ToUpper()

$directoryInstaller = $(Join-Path $directory "\Installer\")
echo "`$directoryInstaller: $directoryInstaller"

$outputIncludeFile = $(Join-Path $directoryInstaller "include.wxi")
If (Test-Path $outputIncludeFile)
{
	Remove-Item $outputIncludeFile
}

$new_output_contents_wxi_include = @"
<?xml version="1.0" encoding="utf-8"?>
<!-- Automatically-generated file. Do not edit! -->
<Include>
  <?define MajorVersion="$Major" ?>
  <?define MinorVersion="$Minor" ?>
  <?define BuildVersion="$Build" ?>
  <?define Version="$Major.$Minor.$Build" ?>
  <?define UpgradeCode="{$UpgradeCode}" ?>
  <?define ProductCode="{$ProductCode}" ?>
  <?define PackageCode="{$PackageCode}" ?>
  <?define ExeProcessName="hexeditor.exe" ?>
  <?define Name = "HexEditor" ?>
  <?define Description = "Binary file editor" ?>
  <?define Manufacturer="chipmunk-sm" ?>
  <?define CopyrightWarning="License GPL v3.0 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details" ?>
  <?define ARPTITLE="HexEditor" ?>
  <?define ARPCONTACT="chipmunk-sm" ?> 
  <?define ARPHELPLINK="https://github.com/chipmunk-sm/HexEditor" ?>
  <?define ARPURLINFOABOUT="https://github.com/chipmunk-sm/HexEditor" ?>
  <?define Platform="$platformId" ?>
</Include>
"@

echo "Update $outputIncludeFile"
[System.IO.File]::WriteAllText( $outputIncludeFile, $new_output_contents_wxi_include, (New-Object System.Text.UTF8Encoding $True))


echo ""
echo "******************************************************"
echo "*** Start update Appveyor Build Version..."
echo ""

if (Get-Command Update-AppveyorBuild -errorAction SilentlyContinue)
{
    Update-AppveyorBuild -Version "$Major.$Minor.$Build"
}

#echo ""
#echo "******************************************************"
#echo "*** Start update MSI props..."
#echo ""

#$proj = $(Join-Path $directory "Installer\hexeditor.$platformId.vdproj")
#echo "`$proj: $proj"
#$fileContent = Get-Content -Path "$proj" -Raw -Encoding UTF8
#if(!$fileContent)
#{
#    Throw "Failed get content from `"$proj`"."
#}

#$replaceTempl = "00000000-0000-0000-0000-000000000000"

##"ProductCode" = "8:{00000000-0000-0000-0000-000000000000}"
#$searchTempl = "`"ProductCode`" = `"8:{"
#$pos = $fileContent.IndexOf($searchTempl) + $searchTempl.Length
#$fileContent = $fileContent.Remove($pos, $replaceTempl.Length).Insert($pos, $ProductCode)

##"PackageCode" = "8:{00000000-0000-0000-0000-000000000000}"
#$searchTempl = "`"PackageCode`" = `"8:{"
#$pos = $fileContent.IndexOf($searchTempl) + $searchTempl.Length
#$fileContent = $fileContent.Remove($pos, $replaceTempl.Length).Insert($pos, $PackageCode)

##"UpgradeCode" = "8:{00000000-0000-0000-0000-000000000000}"
#$searchTempl = "`"UpgradeCode`" = `"8:{"
#$pos = $fileContent.IndexOf($searchTempl) + $searchTempl.Length
#$fileContent = $fileContent.Remove($pos, $replaceTempl.Length).Insert($pos, $UpgradeCode)

#$buildVersion = "$Major.$Minor.$Build"
#echo "`$buildVersion: $buildVersion"
##"ProductVersion" = "8:1.0.0"
#$searchTempl = "`"ProductVersion`" = `"8:"
#$pos = $fileContent.IndexOf($searchTempl) + $searchTempl.Length
#$replaceTempl = "0.0.0"
#$fileContent = $fileContent.Remove($pos, $replaceTempl.Length).Insert($pos, $buildVersion)

#echo "Update $proj"
#[System.IO.File]::WriteAllText( $proj, $fileContent, (New-Object System.Text.UTF8Encoding $True))

echo ""
echo "******************************************************"
echo "*** All Ok! "
echo "******************************************************"


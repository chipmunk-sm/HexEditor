
param(
    [string] $platformId = "x64",
    [string] $cultureId = "en-us",
    [string] $wixRoot = $env:WIX
)

echo ""
echo "******************************************************"
echo "*** Create installer ***"
echo "******************************************************"
echo ""

$directory          = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($PSScriptRoot)
$directoryInstaller = $(Join-Path $directory "Installer")  
$directorySource    = $(Join-Path $directory "Installer\Hexeditor" ) 
$wixBin             = $(Join-Path $wixRoot "bin") 

echo "`$platformId: $platformId"
echo "`$directory: $directory"
echo "`$directoryInstaller: $directoryInstaller"
echo "`$directorySource: $directorySource"
echo "`$wixRoot: $wixRoot"
echo "`$wixBin $wixBin"

#----- wix install dir
if(!$wixRoot){
    Throw "Wix not set. Abort"
}

#----- "sourceFragment.wxs"
$outputFragmentFile = $(Join-Path $directoryInstaller "sourceFragment.wxs")
If (Test-Path $outputFragmentFile){
	Remove-Item $outputFragmentFile
}

#----- "main.wxs" 
$outputMainFile = $(Join-Path $directoryInstaller "main.wxs")
If ( -Not (Test-Path $outputMainFile)){
	Throw "main.wxs not found. Abort"
}

#----- "localization_$cultureId.wxl" 
$localizationFile = $(Join-Path $directoryInstaller "localization_$cultureId.wxl")
If ( -Not (Test-Path $localizationFile)){
	Throw "$localizationFile not found. Abort"
}

#----- "hexeditor.msi" 
$outputMsiFile = $(Join-Path $directoryInstaller "hexeditor.msi")
If (Test-Path $outputMsiFile)
{
	Remove-Item $outputMsiFile
}

echo "----- heat"
& $(Join-Path $wixBin "heat.exe") dir "$directorySource" -cg InstallGroup -dr INSTALLLOCATION -scom -sreg -gg -sfrag -srd -out $outputFragmentFile

echo "----- candle" 
& $(Join-Path $wixBin "candle.exe") $outputMainFile $outputFragmentFile -ext WixUIExtension -ext WiXUtilExtension

echo "----- light"
& $(Join-Path $wixBin "light.exe") -out $outputMsiFile -b $directorySource main.wixobj sourceFragment.wixobj -cultures:$cultureId -loc $localizationFile  -ext WixUIExtension -ext WiXUtilExtension

#dir $directorySource

echo ""
echo "******************************************************"
echo "*** End create installer..."
echo "******************************************************"
echo ""

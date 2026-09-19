[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [Parameter(Mandatory = $true)][string]$ProjectRoot,
    [ValidateRange(1, 16)][int]$MaxParallelActions = 4,
    [switch]$CheckOnly
)
$ErrorActionPreference = 'Stop'
$projectFile = Join-Path $ProjectRoot 'FactoryGame.uproject'
$buildScript = Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat'
$versionFile = Join-Path $EngineRoot 'Engine/Build/Build.version'
if (-not (Test-Path -LiteralPath $buildScript)) { throw "Custom Unreal build tool missing: $buildScript" }
if (-not (Test-Path -LiteralPath $versionFile)) { throw "Engine version file missing: $versionFile" }
$version = Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json
if ($version.MajorVersion -ne 5 -or $version.MinorVersion -ne 6 -or $version.PatchVersion -ne 1 -or $version.BranchName -ne '++5.6.1-CSS') {
    throw 'This port requires the custom Satisfactory Unreal Engine 5.6.1 build.'
}
$project = Get-Content -LiteralPath $projectFile -Raw | ConvertFrom-Json
if ($project.EngineAssociation -ne '5.6.1-CSS') { throw 'Starter Project must target 5.6.1-CSS.' }
if (-not (Test-Path -LiteralPath (Join-Path $ProjectRoot 'Plugins/Wwise/Wwise.uplugin'))) {
    throw 'Integrate Wwise 2023.1.14.8770 into this Starter Project before building.'
}
$wwise = Get-Content -LiteralPath (Join-Path $ProjectRoot 'Plugins/Wwise/Wwise.uplugin') -Raw | ConvertFrom-Json
if ($wwise.VersionName -notlike '2023.1.14.8770*') { throw 'Wwise integration must use 2023.1.14.8770.' }
if (-not (Test-Path -LiteralPath (Join-Path $ProjectRoot 'Plugins/Wwise/ThirdParty/x64_vc170/Profile/lib/AkSoundEngine.lib'))) {
    throw 'The Wwise Visual Studio 2022 x64 SDK library is missing; wait for integration to finish or add that SDK platform.'
}
if (-not (Test-Path -LiteralPath (Join-Path $ProjectRoot 'Mods/HyperManage/HyperManage.uplugin'))) {
    throw 'HyperManage must be linked or copied into the Starter Project Mods folder.'
}
if ($CheckOnly) { Write-Output 'Basic project prerequisites found. A successful compile is still required.'; exit 0 }
$projectPath = (Resolve-Path -LiteralPath $projectFile).Path
& $buildScript FactoryEditor Win64 Development "-Project=$projectPath" -WaitMutex -NoHotReloadFromIDE '-Compiler=VisualStudio2022' '-CompilerVersion=14.38.33130' "-MaxParallelActions=$MaxParallelActions"
if ($LASTEXITCODE -ne 0) { throw "Unreal build failed with exit code $LASTEXITCODE" }

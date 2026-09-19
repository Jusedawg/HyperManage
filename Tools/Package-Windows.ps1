[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [Parameter(Mandatory = $true)][string]$ProjectRoot,
    [ValidateRange(1, 16)][int]$MaxParallelActions = 4
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'Build-Editor.ps1') -EngineRoot $EngineRoot -ProjectRoot $ProjectRoot -CheckOnly
$project = (Resolve-Path -LiteralPath (Join-Path $ProjectRoot 'FactoryGame.uproject')).Path
$uat = Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
& $uat "-ScriptsForProject=$project" PackagePlugin "-project=$project" '-clientconfig=Shipping' '-serverconfig=Shipping' '-utf8output' '-DLCName=HyperManage' '-build' '-platform=Win64' '-nocompileeditor' '-installed' "-UbtArgs=-Compiler=VisualStudio2022 -CompilerVersion=14.38.33130 -MaxParallelActions=$MaxParallelActions"
if ($LASTEXITCODE -ne 0) { throw "Alpakit Windows packaging failed with exit code $LASTEXITCODE" }
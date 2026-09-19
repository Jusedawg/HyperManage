[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [Parameter(Mandatory = $true)][string]$ProjectRoot
)
$ErrorActionPreference = 'Stop'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $ProjectRoot 'FactoryGame.uproject'
$pluginRoot = Split-Path -Parent $PSScriptRoot
$contentRoot = (Resolve-Path -LiteralPath (Join-Path $pluginRoot 'Content')).Path
$expectedBlueprints = @(
    '/HyperManage/Equipment/Desc_HyperManager.Desc_HyperManager',
    '/HyperManage/Recipes/HyperManager/Recipe_HyperManager.Recipe_HyperManager',
    '/HyperManage/Schematics/SC_MM.SC_MM',
    '/HyperManage/Schematics/Schematic_HyperManage.Schematic_HyperManage',
    '/HyperManage/UI/MMInfoWidget.MMInfoWidget',
    '/HyperManage/UI/MMPlaceholder.MMPlaceholder',
    '/HyperManage/UI/MMSettings.MMSettings',
    '/HyperManage/UI/MMToolsUI.MMToolsUI'
)
if (-not (Test-Path -LiteralPath (Join-Path $pluginRoot 'Binaries/Win64/UnrealEditor-HyperManage.dll'))) {
    throw 'Build the HyperManage editor module successfully before validating Blueprints.'
}
$savedDir = Join-Path $ProjectRoot 'Saved'
New-Item -ItemType Directory -Force -Path $savedDir | Out-Null
$allowList = Join-Path $savedDir 'HyperManageBlueprintAllowList.txt'
$expectedBlueprints | Set-Content -LiteralPath $allowList -Encoding ascii
$logPath = Join-Path (Resolve-Path -LiteralPath $savedDir).Path 'HyperManage-BlueprintValidation.log'
& $editor (Resolve-Path -LiteralPath $project).Path '-run=CompileAllBlueprints' '-AllowListFile=Saved/HyperManageBlueprintAllowList.txt' '-unattended' '-nop4' '-nullrhi' '-nosplash' '-stdout' "-abslog=$logPath"
$compileExit = $LASTEXITCODE
if ($compileExit -ne 0) { throw "Blueprint validation failed with exit code $compileExit. See $logPath" }
$logText = Get-Content -LiteralPath $logPath -Raw
foreach ($asset in $expectedBlueprints) {
    if (-not $logText.Contains("Loading and Compiling: '$asset'")) { throw "Blueprint was not checked: $asset. See $logPath" }
}
Write-Output "All $($expectedBlueprints.Count) expected Blueprints were compiled. Review warnings in $logPath"

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [Parameter(Mandatory = $true)][string]$ProjectRoot
)
$ErrorActionPreference = 'Stop'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = (Resolve-Path -LiteralPath (Join-Path $ProjectRoot 'FactoryGame.uproject')).Path
$log = Join-Path (Resolve-Path -LiteralPath (Join-Path $ProjectRoot 'Saved')).Path 'HyperManage-Automation.log'
& $editor $project '-unattended' '-nop4' '-nullrhi' '-nosplash' '-stdout' '-FORCELOGFLUSH' '-ExecCmds=Automation RunTests HyperManage.' '-TestExit=Automation Test Queue Empty' "-abslog=$log"
if ($LASTEXITCODE -ne 0) { throw "Automation failed: $LASTEXITCODE. See $log" }
$report = Get-Content -LiteralPath $log -Raw
foreach ($test in @('StaleIdentity', 'TransformBounds', 'PivotTransforms', 'AttachmentRoot', 'PreservesMaterials', 'WorldAlignment', 'NestedToolbar', 'OriginalClipboard', 'PrecisionValues', 'UndoRedo', 'WorldOffset', 'WorldRotationOffset')) {
    if ($report -notmatch "Test Completed\. Result=\{Success\} Name=\{$test\}") { throw "Test $test did not pass. See $log" }
}
Write-Output 'All twelve HyperManage regression tests passed.'

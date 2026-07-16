[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[A-Za-z0-9_-]+$')]
    [string]$Label,

    [ValidateRange(10, 600)]
    [int]$DurationSeconds = 60,

    [ValidateRange(0, 60)]
    [int]$DelaySeconds = 5,

    [string]$PresentMonPath = 'D:\AllDownload\PresentMon-2.5.1-x64.exe',

    [string]$OutputDirectory = 'D:\AllDownload\PalworldResourceESP-Benchmarks'
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $PresentMonPath -PathType Leaf)) {
    throw "PresentMon executable not found: $PresentMonPath"
}

$processes = @(Get-Process -Name 'Palworld-Win64-Shipping' -ErrorAction SilentlyContinue)
if ($processes.Count -ne 1) {
    throw "Expected exactly one Palworld process, found $($processes.Count)."
}
$process = $processes[0]

if (-not (Test-Path -LiteralPath $OutputDirectory -PathType Container)) {
    New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
}

$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$outputPath = Join-Path $OutputDirectory "$timestamp-$Label.csv"
if (Test-Path -LiteralPath $outputPath) {
    throw "Capture output already exists: $outputPath"
}

$sessionName = "PalworldResourceESP-$($process.Id)-$timestamp"
$captureRequestedAt = Get-Date
$captureStartsAt = $captureRequestedAt.AddSeconds($DelaySeconds)
$captureEndsAt = $captureStartsAt.AddSeconds($DurationSeconds)
$arguments = @(
    '--process_id', $process.Id,
    '--output_file', $outputPath,
    '--v2_metrics',
    '--timed', $DurationSeconds,
    '--delay', $DelaySeconds,
    '--terminate_after_timed',
    '--terminate_on_proc_exit',
    '--no_console_stats',
    '--session_name', $sessionName
)

Write-Host "PresentMon target PID: $($process.Id)"
Write-Host "Capture starts after $DelaySeconds seconds and runs for $DurationSeconds seconds."
Write-Host "Output: $outputPath"

& $PresentMonPath @arguments
if ($LASTEXITCODE -ne 0) {
    throw "PresentMon exited with code $LASTEXITCODE."
}
if (-not (Test-Path -LiteralPath $outputPath -PathType Leaf)) {
    throw "PresentMon did not create the expected CSV: $outputPath"
}

$outputFile = Get-Item -LiteralPath $outputPath
[pscustomobject]@{
    Label = $Label
    ProcessId = $process.Id
    DurationSeconds = $DurationSeconds
    DelaySeconds = $DelaySeconds
    CaptureStartsAt = $captureStartsAt
    CaptureEndsAt = $captureEndsAt
    OutputPath = $outputFile.FullName
    SizeBytes = $outputFile.Length
    CapturedAt = $outputFile.LastWriteTime
}

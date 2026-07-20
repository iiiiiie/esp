[CmdletBinding()]
param(
    [ValidatePattern('^[A-Za-z0-9_-]+$')]
    [string]$Label = 'panel-experiment',

    [string]$PresentMonPath = 'D:\AllDownload\PresentMon-2.5.1-x64.exe',

    [string]$OutputDirectory = 'D:\AllDownload\PalworldResourceESP-Benchmarks',

    [string]$Ue4ssLogPath = 'E:\steam\steamapps\common\Palworld\Pal\Binaries\Win64\ue4ss\UE4SS.log',

    [ValidateRange(100, 5000)]
    [int]$PollMilliseconds = 250,

    [ValidateRange(0, 86400)]
    [int]$ProcessWaitTimeoutSeconds = 0
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'PanelCapture.Common.ps1')

function Wait-PalworldProcess {
    param([int]$TimeoutSeconds, [int]$PollIntervalMilliseconds)

    $startedAt = Get-Date
    while ($true) {
        $processes = @(Get-Process -Name 'Palworld-Win64-Shipping' -ErrorAction SilentlyContinue)
        if ($processes.Count -eq 1) {
            return $processes[0]
        }
        if ($processes.Count -gt 1) {
            throw "Expected at most one Palworld process, found $($processes.Count)."
        }
        if ($TimeoutSeconds -gt 0 -and ((Get-Date) - $startedAt).TotalSeconds -ge $TimeoutSeconds) {
            throw "Timed out waiting for Palworld after $TimeoutSeconds seconds."
        }
        Start-Sleep -Milliseconds $PollIntervalMilliseconds
    }
}

function Read-SharedTextFromOffset {
    param(
        [string]$Path,
        [ref]$Offset
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return ''
    }

    $file = Get-Item -LiteralPath $Path
    if ($file.Length -lt $Offset.Value) {
        $Offset.Value = 0L
    }
    if ($file.Length -eq $Offset.Value) {
        return ''
    }

    $share = [System.IO.FileShare]::ReadWrite -bor [System.IO.FileShare]::Delete
    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, $share)
    try {
        [void]$stream.Seek($Offset.Value, [System.IO.SeekOrigin]::Begin)
        $reader = New-Object System.IO.StreamReader($stream, [System.Text.Encoding]::UTF8, $true, 4096, $true)
        try {
            $text = $reader.ReadToEnd()
            $Offset.Value = $stream.Position
            return $text
        } finally {
            $reader.Dispose()
        }
    } finally {
        $stream.Dispose()
    }
}

function Stop-PresentMonSession {
    param([string]$Executable, [string]$SessionName)

    $terminator = Start-Process -FilePath $Executable -ArgumentList @(
        '--session_name', $SessionName,
        '--terminate_existing_session',
        '--no_console_stats'
    ) -WindowStyle Hidden -PassThru -Wait
    if ($terminator.ExitCode -ne 0) {
        throw "PresentMon session terminator exited with code $($terminator.ExitCode)."
    }
}

if (-not (Test-Path -LiteralPath $PresentMonPath -PathType Leaf)) {
    throw "PresentMon executable not found: $PresentMonPath"
}
if (-not (Test-Path -LiteralPath (Split-Path -Parent $OutputDirectory) -PathType Container)) {
    throw "Benchmark parent directory not found: $(Split-Path -Parent $OutputDirectory)"
}
if (-not (Test-Path -LiteralPath $OutputDirectory -PathType Container)) {
    New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
}

Write-Host 'Waiting for Palworld-Win64-Shipping...'
$gameProcess = Wait-PalworldProcess $ProcessWaitTimeoutSeconds $PollMilliseconds
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$baseName = "$timestamp-$Label"
$csvPath = Join-Path $OutputDirectory "$baseName.csv"
$metadataPath = Join-Path $OutputDirectory "$baseName.segments.json"
foreach ($path in @($csvPath, $metadataPath)) {
    if (Test-Path -LiteralPath $path) {
        throw "Capture output already exists: $path"
    }
}

$sessionName = "PalworldResourceESP-$($gameProcess.Id)-$timestamp"
$presentMonArguments = @(
    '--process_id', $gameProcess.Id,
    '--output_file', ('"{0}"' -f $csvPath),
    '--v2_metrics',
    '--date_time',
    '--terminate_on_proc_exit',
    '--no_console_stats',
    '--session_name', $sessionName
)
$presentMonStartedAt = [datetimeoffset](Get-Date)
$captureProcess = Start-Process -FilePath $PresentMonPath -ArgumentList $presentMonArguments -WindowStyle Hidden -PassThru
Start-Sleep -Milliseconds 500
$captureProcess.Refresh()
if ($captureProcess.HasExited) {
    throw "PresentMon exited before capture began with code $($captureProcess.ExitCode)."
}

Write-Host "PresentMon target PID: $($gameProcess.Id)"
Write-Host "Session: $sessionName"
Write-Host "CSV: $csvPath"
Write-Host 'Use the panel Start capture and Stop capture buttons to delimit the experiment.'

$markers = New-Object System.Collections.ArrayList
$seenMarkers = @{}
$logOffset = 0L
$logBuffer = ''
$activeCaptureSession = $null
$stoppedAt = $null
$stopReason = 'watcher_error'
$notBefore = $presentMonStartedAt.AddSeconds(-2)

try {
    while ($true) {
        $gameProcess.Refresh()
        $captureProcess.Refresh()
        if ($gameProcess.HasExited) {
            $stopReason = 'process_exit'
            break
        }
        if ($captureProcess.HasExited) {
            throw "PresentMon exited unexpectedly with code $($captureProcess.ExitCode)."
        }

        try {
            $newText = Read-SharedTextFromOffset $Ue4ssLogPath ([ref]$logOffset)
        } catch [System.IO.IOException] {
            $newText = ''
        }
        if ($newText.Length -gt 0) {
            $logBuffer += $newText
            foreach ($marker in @(ConvertFrom-EspPerformanceMarkerText -Text $logBuffer -NotBefore $notBefore)) {
                $key = '{0:o}|{1}|{2}|{3}|{4}' -f $marker.Timestamp, $marker.Event,
                    $marker.CaptureSession, $marker.Profile, $marker.Reason
                if ($seenMarkers.ContainsKey($key)) {
                    continue
                }
                $seenMarkers[$key] = $true
                [void]$markers.Add($marker)
                Write-Host ("Marker: {0:o} {1} session={2} profile={3} preset={4}" -f
                    $marker.Timestamp, $marker.Event, $marker.CaptureSession, $marker.Profile, $marker.Preset)

                if ($marker.Event -eq 'PERF_SESSION_START') {
                    $activeCaptureSession = $marker.CaptureSession
                } elseif (
                    $marker.Event -eq 'PERF_SESSION_STOP' -and
                    $null -ne $activeCaptureSession -and
                    $marker.CaptureSession -eq $activeCaptureSession
                ) {
                    $stoppedAt = $marker.Timestamp
                    $stopReason = 'panel_stop'
                    Stop-PresentMonSession $PresentMonPath $sessionName
                    [void]$captureProcess.WaitForExit(10000)
                    break
                }
            }
            if ($logBuffer.Length -gt 16384) {
                $logBuffer = $logBuffer.Substring($logBuffer.Length - 4096)
            }
        }
        if ($stopReason -eq 'panel_stop') {
            break
        }
        Start-Sleep -Milliseconds $PollMilliseconds
    }
} finally {
    $captureProcess.Refresh()
    if (-not $captureProcess.HasExited) {
        Stop-PresentMonSession $PresentMonPath $sessionName
        [void]$captureProcess.WaitForExit(10000)
    }

    $metadataMarkers = @($markers | ForEach-Object {
        [ordered]@{
            Event = $_.Event
            Timestamp = $_.Timestamp.ToString('o')
            CaptureSession = $_.CaptureSession
            Profile = $_.Profile
            Preset = $_.Preset
            Reason = $_.Reason
        }
    })
    $metadata = [ordered]@{
        SchemaVersion = 1
        SessionName = $sessionName
        ProcessId = $gameProcess.Id
        CsvPath = $csvPath
        PresentMonStartedAt = $presentMonStartedAt.ToString('o')
        StoppedAt = if ($null -ne $stoppedAt) { $stoppedAt.ToString('o') } else { $null }
        StopReason = $stopReason
        Markers = $metadataMarkers
    }
    $metadata | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $metadataPath -Encoding UTF8
}

if (-not (Test-Path -LiteralPath $csvPath -PathType Leaf)) {
    throw "PresentMon did not create the expected CSV: $csvPath"
}

[pscustomobject]@{
    ProcessId = $gameProcess.Id
    SessionName = $sessionName
    StopReason = $stopReason
    MarkerCount = $markers.Count
    CsvPath = $csvPath
    MetadataPath = $metadataPath
}

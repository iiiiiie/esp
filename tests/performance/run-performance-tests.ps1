$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
. (Join-Path $repoRoot 'tools\performance\PanelCapture.Common.ps1')

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) {
        throw "$Message (expected=$Expected actual=$Actual)"
    }
}

$logText = @'
[2026-07-17 10:00:00.0000000] [Lua] [PalworldResourceESP] PERF_SESSION_START capture_session=7 reason=panel_revision[2026-07-17 10:00:00.1000000] [Lua] [PalworldResourceESP] PERF_MODE_CHANGED capture_session=7 profile=off preset=balanced reason=capture_start
[2026-07-17 10:00:15.1000000] [Lua] [PalworldResourceESP] PERF_MODE_CHANGED capture_session=7 profile=snapshot_once preset=balanced reason=profile_change[2026-07-17 10:00:30.1000000] [Lua] [PalworldResourceESP] PERF_MODE_CHANGED capture_session=7 profile=chunked_current preset=balanced reason=profile_change
[2026-07-17 10:00:45.1000000] [Lua] [PalworldResourceESP] PERF_MODE_CHANGED capture_session=7 profile=event_first preset=balanced reason=profile_change[2026-07-17 10:01:00.1000000] [Lua] [PalworldResourceESP] PERF_SESSION_STOP capture_session=7 profile=event_first reason=panel_revision
'@

$markers = @(ConvertFrom-EspPerformanceMarkerText -Text $logText)
Assert-Equal $markers.Count 6 'Concatenated UE4SS markers were not parsed'
Assert-Equal $markers[0].CaptureSession 7 'Capture session was not parsed'
Assert-Equal $markers[4].Profile 'event_first' 'Profile was not parsed'

$filtered = @(ConvertFrom-EspPerformanceMarkerText -Text $logText -NotBefore (
    ConvertTo-EspTimestamp '2026-07-17 10:00:30.0000000'
))
Assert-Equal $filtered.Count 3 'NotBefore did not filter old markers'

$segments = @(Get-EspPerformanceSegments -Markers $markers -TransitionSeconds 2)
Assert-Equal $segments.Count 4 'Expected four performance segments'
Assert-Equal $segments[0].Profile 'off' 'First segment mode is incorrect'
Assert-Equal $segments[3].Profile 'event_first' 'Last segment mode is incorrect'
Assert-Equal $segments[0].DurationSeconds 13 'Transition interval was not excluded'
Assert-Equal $segments[0].AnalysisStartsAt.ToString('HH:mm:ss.fff') '10:00:02.100' 'Analysis start is incorrect'

$missingStop = @($markers | Where-Object { $_.Event -ne 'PERF_SESSION_STOP' })
$threw = $false
try {
    [void]@(Get-EspPerformanceSegments -Markers $missingStop)
} catch {
    $threw = $true
}
Assert-Equal $threw $true 'Missing stop marker did not fail closed'

$analysis = & (Join-Path $repoRoot 'tools\performance\Measure-PresentMon.ps1') `
    -CandidateCsv (Join-Path $PSScriptRoot 'segmented-presentmon.csv') `
    -SegmentMetadataPath (Join-Path $PSScriptRoot 'segmented-presentmon.segments.json')
Assert-Equal $analysis.Segments.Count 1 'Segmented analysis did not return one segment'
Assert-Equal $analysis.Segments[0].Statistics.Frames 31 'Segmented analysis selected the wrong frames'
Assert-Equal $analysis.Segments[0].Statistics.StartTimeSource 'CPUStartDateTime' 'Absolute time source was not used'
Assert-Equal $analysis.Segments[0].Statistics.Hitch50Count 1 'Segmented hitch count is incorrect'

Write-Host 'PASS performance marker parsing'
Write-Host 'PASS performance segment transition exclusion'
Write-Host 'PASS absolute DateTime segment analysis'

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$CandidateCsv,

    [string]$BaselineCsv,

    [string]$Application,

    [ValidateRange(0, 120)]
    [double]$WarmupSeconds = 0,

    [string]$SegmentMetadataPath,

    [ValidateRange(0, 30)]
    [double]$TransitionSeconds = 2,

    [switch]$AsJson
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'PanelCapture.Common.ps1')
$invariantCulture = [System.Globalization.CultureInfo]::InvariantCulture
$numberStyles = [System.Globalization.NumberStyles]::Float

function Convert-ToDouble {
    param([string]$Value)

    $parsed = 0.0
    if (-not [double]::TryParse($Value, $numberStyles, $invariantCulture, [ref]$parsed)) {
        return $null
    }
    return $parsed
}

function Get-Percentile {
    param(
        [double[]]$SortedValues,
        [ValidateRange(0, 100)]
        [double]$Percentile
    )

    if ($SortedValues.Count -eq 0) {
        throw 'Cannot calculate a percentile for an empty sample.'
    }
    if ($SortedValues.Count -eq 1) {
        return $SortedValues[0]
    }

    $position = ($Percentile / 100.0) * ($SortedValues.Count - 1)
    $lower = [math]::Floor($position)
    $upper = [math]::Ceiling($position)
    if ($lower -eq $upper) {
        return $SortedValues[$lower]
    }

    $weight = $position - $lower
    return $SortedValues[$lower] + (($SortedValues[$upper] - $SortedValues[$lower]) * $weight)
}

function Get-PresentMonStatistics {
    param(
        [string]$CsvPath,
        [string]$ApplicationFilter,
        [double]$Warmup,
        [object[]]$Intervals = @()
    )

    if (-not (Test-Path -LiteralPath $CsvPath -PathType Leaf)) {
        throw "PresentMon CSV not found: $CsvPath"
    }

    $rows = @(Import-Csv -LiteralPath $CsvPath)
    if ($rows.Count -eq 0) {
        throw "PresentMon CSV contains no frames: $CsvPath"
    }
    foreach ($requiredColumn in @('Application', 'ProcessID', 'SwapChainAddress', 'FrameTime')) {
        if ($requiredColumn -notin $rows[0].PSObject.Properties.Name) {
            throw "PresentMon CSV is missing required column '$requiredColumn': $CsvPath"
        }
    }

    if ($ApplicationFilter) {
        $rows = @($rows | Where-Object { $_.Application -ieq $ApplicationFilter })
        if ($rows.Count -eq 0) {
            throw "No rows matched application '$ApplicationFilter': $CsvPath"
        }
    }

    $primaryGroup = $rows |
        Group-Object -Property Application, ProcessID, SwapChainAddress |
        Sort-Object -Property Count -Descending |
        Select-Object -First 1
    $selectedRows = @($primaryGroup.Group)

    $hasCpuStartDateTime = 'CPUStartDateTime' -in $rows[0].PSObject.Properties.Name
    $hasCpuStartTime = 'CPUStartTime' -in $rows[0].PSObject.Properties.Name
    $cumulativeStartMs = 0.0

    $samples = foreach ($row in $selectedRows) {
        $startMs = $null
        $startDateTime = $null
        $frameTimeMs = Convert-ToDouble $row.FrameTime
        if ($null -ne $frameTimeMs -and $frameTimeMs -gt 0) {
            if ($hasCpuStartDateTime) {
                $startDateTime = ConvertTo-EspTimestamp $row.CPUStartDateTime
            }
            $startMs = if ($hasCpuStartTime) {
                Convert-ToDouble $row.CPUStartTime
            } else {
                $cumulativeStartMs
            }
            $cumulativeStartMs += $frameTimeMs
        }
        if ($null -ne $startMs -and $null -ne $frameTimeMs -and $frameTimeMs -gt 0) {
            [pscustomobject]@{
                StartMs = $startMs
                StartDateTime = $startDateTime
                FrameTimeMs = $frameTimeMs
            }
        }
    }
    $samples = if ($hasCpuStartDateTime) {
        @($samples | Sort-Object -Property StartDateTime)
    } else {
        @($samples | Sort-Object -Property StartMs)
    }
    if ($Intervals.Count -gt 0) {
        if (-not $hasCpuStartDateTime) {
            throw "Segment analysis requires the CPUStartDateTime column. Capture with PresentMon --date_time: $CsvPath"
        }
        $samples = @($samples | Where-Object {
            $sample = $_
            @($Intervals | Where-Object {
                $intervalStart = if ($_.AnalysisStartsAt -is [datetimeoffset]) {
                    $_.AnalysisStartsAt
                } else {
                    ConvertTo-EspTimestamp ([string]$_.AnalysisStartsAt)
                }
                $intervalEnd = if ($_.EndsAt -is [datetimeoffset]) {
                    $_.EndsAt
                } else {
                    ConvertTo-EspTimestamp ([string]$_.EndsAt)
                }
                $sample.StartDateTime -ge $intervalStart -and $sample.StartDateTime -lt $intervalEnd
            }).Count -gt 0
        })
    } elseif ($samples.Count -gt 0 -and $Warmup -gt 0) {
        if ($hasCpuStartDateTime) {
            $warmupEndsAt = $samples[0].StartDateTime.AddSeconds($Warmup)
            $samples = @($samples | Where-Object { $_.StartDateTime -ge $warmupEndsAt })
        } else {
            $warmupEndsAtMs = $samples[0].StartMs + ($Warmup * 1000.0)
            $samples = @($samples | Where-Object { $_.StartMs -ge $warmupEndsAtMs })
        }
    }
    if ($samples.Count -lt 30) {
        throw "Fewer than 30 usable frames remain after filtering: $CsvPath"
    }

    [double[]]$sortedFrameTimes = @($samples.FrameTimeMs | Sort-Object)
    $averageMs = ($sortedFrameTimes | Measure-Object -Average).Average
    $durationSeconds = ($sortedFrameTimes | Measure-Object -Sum).Sum / 1000.0
    if ($durationSeconds -le 0) {
        throw "PresentMon samples have no measurable duration: $CsvPath"
    }

    $p50 = Get-Percentile $sortedFrameTimes 50
    $p95 = Get-Percentile $sortedFrameTimes 95
    $p99 = Get-Percentile $sortedFrameTimes 99
    $p999 = Get-Percentile $sortedFrameTimes 99.9
    $hitch33 = @($sortedFrameTimes | Where-Object { $_ -gt 33.33 }).Count
    $hitch50 = @($sortedFrameTimes | Where-Object { $_ -gt 50.0 }).Count
    $hitch100 = @($sortedFrameTimes | Where-Object { $_ -gt 100.0 }).Count
    $minutes = $durationSeconds / 60.0

    [pscustomobject]@{
        CsvPath = (Resolve-Path -LiteralPath $CsvPath).Path
        Application = $selectedRows[0].Application
        ProcessId = [int]$selectedRows[0].ProcessID
        SwapChainAddress = $selectedRows[0].SwapChainAddress
        StartTimeSource = if ($hasCpuStartDateTime) {
            'CPUStartDateTime'
        } elseif ($hasCpuStartTime) {
            'CPUStartTime'
        } else {
            'cumulative FrameTime'
        }
        Frames = $samples.Count
        DurationSeconds = [math]::Round($durationSeconds, 3)
        AverageFps = [math]::Round(1000.0 / $averageMs, 3)
        AverageFrameTimeMs = [math]::Round($averageMs, 3)
        P50FrameTimeMs = [math]::Round($p50, 3)
        P95FrameTimeMs = [math]::Round($p95, 3)
        P99FrameTimeMs = [math]::Round($p99, 3)
        P99_9FrameTimeMs = [math]::Round($p999, 3)
        OnePercentLowFpsApprox = [math]::Round(1000.0 / $p99, 3)
        PointOnePercentLowFpsApprox = [math]::Round(1000.0 / $p999, 3)
        Hitch33Count = $hitch33
        Hitch50Count = $hitch50
        Hitch100Count = $hitch100
        Hitch33PerMinute = [math]::Round($hitch33 / $minutes, 3)
        Hitch50PerMinute = [math]::Round($hitch50 / $minutes, 3)
        Hitch100PerMinute = [math]::Round($hitch100 / $minutes, 3)
    }
}

function Get-PercentDelta {
    param([double]$Baseline, [double]$Candidate)
    if ($Baseline -eq 0) {
        return $null
    }
    return (($Candidate - $Baseline) / $Baseline) * 100.0
}

$candidate = $null
if ($SegmentMetadataPath) {
    if ($BaselineCsv) {
        throw 'SegmentMetadataPath cannot be combined with BaselineCsv. Analyze the segmented capture separately.'
    }
    if (-not (Test-Path -LiteralPath $SegmentMetadataPath -PathType Leaf)) {
        throw "Segment metadata not found: $SegmentMetadataPath"
    }
    $metadata = Get-Content -Raw -LiteralPath $SegmentMetadataPath | ConvertFrom-Json
    $markers = @($metadata.Markers | ForEach-Object {
        [pscustomobject]@{
            Event = $_.Event
            Timestamp = ConvertTo-EspTimestamp ([string]$_.Timestamp)
            CaptureSession = $_.CaptureSession
            Profile = $_.Profile
            Preset = $_.Preset
            Reason = $_.Reason
        }
    })
    $segments = @(Get-EspPerformanceSegments -Markers $markers -TransitionSeconds $TransitionSeconds)
    $segmentResults = @($segments | ForEach-Object {
        $segment = $_
        [pscustomobject]@{
            Sequence = $segment.Sequence
            CaptureSession = $segment.CaptureSession
            Profile = $segment.Profile
            Preset = $segment.Preset
            StartedAt = $segment.StartedAt
            AnalysisStartsAt = $segment.AnalysisStartsAt
            EndsAt = $segment.EndsAt
            TransitionSeconds = $segment.TransitionSeconds
            Statistics = Get-PresentMonStatistics $CandidateCsv $Application 0 @($segment)
        }
    })
    $result = [pscustomobject]@{
        CandidateCsv = (Resolve-Path -LiteralPath $CandidateCsv).Path
        SegmentMetadataPath = (Resolve-Path -LiteralPath $SegmentMetadataPath).Path
        TransitionSeconds = $TransitionSeconds
        Segments = $segmentResults
    }
} else {
    $candidate = Get-PresentMonStatistics $CandidateCsv $Application $WarmupSeconds
}
if ($SegmentMetadataPath) {
    # The segmented result is complete above.
} elseif (-not $BaselineCsv) {
    $result = $candidate
} else {
    $baseline = Get-PresentMonStatistics $BaselineCsv $Application $WarmupSeconds
    $p95Delta = Get-PercentDelta $baseline.P95FrameTimeMs $candidate.P95FrameTimeMs
    $p99Delta = Get-PercentDelta $baseline.P99FrameTimeMs $candidate.P99FrameTimeMs
    $hitch50Delta = $candidate.Hitch50PerMinute - $baseline.Hitch50PerMinute
    $additionalHitch100 = $candidate.Hitch100Count - $baseline.Hitch100Count

    $comparison = [pscustomobject]@{
        P95DeltaPercent = [math]::Round($p95Delta, 3)
        P99DeltaPercent = [math]::Round($p99Delta, 3)
        Hitch50PerMinuteDelta = [math]::Round($hitch50Delta, 3)
        AdditionalHitch100Count = $additionalHitch100
        P95GatePass = $p95Delta -le 5.0
        P99GatePass = $p99Delta -le 10.0
        Hitch50GatePass = $hitch50Delta -le 2.0
        Hitch100GatePass = $additionalHitch100 -le 0
    }
    $comparison | Add-Member -NotePropertyName Pass -NotePropertyValue (
        $comparison.P95GatePass -and
        $comparison.P99GatePass -and
        $comparison.Hitch50GatePass -and
        $comparison.Hitch100GatePass
    )

    $result = [pscustomobject]@{
        Baseline = $baseline
        Candidate = $candidate
        Comparison = $comparison
    }
}

if ($AsJson) {
    $result | ConvertTo-Json -Depth 5
} else {
    $result
}

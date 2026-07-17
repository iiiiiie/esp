function ConvertTo-EspTimestamp {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Value
    )

    $normalized = $Value.Trim()
    $nanosecondMatch = [regex]::Match(
        $normalized,
        '^(?<Prefix>.*\.\d{7})\d+(?<Suffix>(?:Z|[+-]\d{2}:?\d{2})?)$'
    )
    if ($nanosecondMatch.Success) {
        $normalized = $nanosecondMatch.Groups['Prefix'].Value + $nanosecondMatch.Groups['Suffix'].Value
    }

    $parsed = [datetimeoffset]::MinValue
    $styles = [System.Globalization.DateTimeStyles]::AllowWhiteSpaces -bor
        [System.Globalization.DateTimeStyles]::AssumeLocal
    if (-not [datetimeoffset]::TryParse(
        $normalized,
        [System.Globalization.CultureInfo]::InvariantCulture,
        $styles,
        [ref]$parsed
    )) {
        throw "Invalid performance timestamp: $Value"
    }
    return $parsed
}

function ConvertFrom-EspMarkerFields {
    param([string]$Text)

    $fields = @{}
    foreach ($match in [regex]::Matches($Text, '(?<Key>[A-Za-z_][A-Za-z0-9_]*)=(?<Value>[^\s\[]+)')) {
        $fields[$match.Groups['Key'].Value] = $match.Groups['Value'].Value
    }
    return $fields
}

function ConvertFrom-EspPerformanceMarkerText {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text,

        [datetimeoffset]$NotBefore = [datetimeoffset]::MinValue
    )

    $timestampPattern = '\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{1,9}'
    $pattern = "\[(?<Timestamp>$timestampPattern)\][^\r\n]*?\[PalworldResourceESP\]\s+" +
        '(?<Event>PERF_SESSION_START|PERF_MODE_CHANGED|PERF_SESSION_STOP)' +
        "\s*(?<Fields>[^\r\n]*?)(?=(?:\[$timestampPattern\]|\r?\n|$))"

    foreach ($match in [regex]::Matches($Text, $pattern)) {
        $timestamp = ConvertTo-EspTimestamp $match.Groups['Timestamp'].Value
        if ($timestamp -lt $NotBefore) {
            continue
        }

        $fields = ConvertFrom-EspMarkerFields $match.Groups['Fields'].Value
        $captureSession = $null
        if ($fields.ContainsKey('capture_session')) {
            $parsedSession = 0
            if ([int]::TryParse($fields['capture_session'], [ref]$parsedSession)) {
                $captureSession = $parsedSession
            }
        }

        [pscustomobject]@{
            Event = $match.Groups['Event'].Value
            Timestamp = $timestamp
            CaptureSession = $captureSession
            Profile = if ($fields.ContainsKey('profile')) { $fields['profile'] } else { $null }
            Preset = if ($fields.ContainsKey('preset')) { $fields['preset'] } else { $null }
            Reason = if ($fields.ContainsKey('reason')) { $fields['reason'] } else { $null }
            RawFields = $match.Groups['Fields'].Value.Trim()
        }
    }
}

function Get-EspPerformanceSegments {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [object[]]$Markers,

        [ValidateRange(0, 30)]
        [double]$TransitionSeconds = 2
    )

    $ordered = @($Markers | Sort-Object -Property Timestamp)
    $sessionStart = $ordered | Where-Object { $_.Event -eq 'PERF_SESSION_START' } | Select-Object -First 1
    if ($null -eq $sessionStart) {
        throw 'Performance metadata has no PERF_SESSION_START marker.'
    }

    $captureSession = $sessionStart.CaptureSession
    $sessionMarkers = @($ordered | Where-Object {
        $_.Timestamp -ge $sessionStart.Timestamp -and
        ($null -eq $captureSession -or $_.CaptureSession -eq $captureSession)
    })
    $sessionStop = $sessionMarkers |
        Where-Object { $_.Event -eq 'PERF_SESSION_STOP' } |
        Select-Object -First 1
    if ($null -eq $sessionStop) {
        throw 'Performance metadata has no matching PERF_SESSION_STOP marker.'
    }

    $modeMarkers = @($sessionMarkers | Where-Object {
        $_.Event -eq 'PERF_MODE_CHANGED' -and
        $_.Timestamp -lt $sessionStop.Timestamp
    })
    if ($modeMarkers.Count -eq 0) {
        throw 'Performance metadata has no PERF_MODE_CHANGED markers.'
    }

    for ($index = 0; $index -lt $modeMarkers.Count; $index++) {
        $mode = $modeMarkers[$index]
        $endsAt = if ($index + 1 -lt $modeMarkers.Count) {
            $modeMarkers[$index + 1].Timestamp
        } else {
            $sessionStop.Timestamp
        }
        $analysisStartsAt = $mode.Timestamp.AddSeconds($TransitionSeconds)
        if ($analysisStartsAt -ge $endsAt) {
            continue
        }

        [pscustomobject]@{
            Sequence = $index + 1
            CaptureSession = $captureSession
            Profile = $mode.Profile
            Preset = $mode.Preset
            StartedAt = $mode.Timestamp
            AnalysisStartsAt = $analysisStartsAt
            EndsAt = $endsAt
            TransitionSeconds = $TransitionSeconds
            DurationSeconds = [math]::Round(($endsAt - $analysisStartsAt).TotalSeconds, 3)
        }
    }
}

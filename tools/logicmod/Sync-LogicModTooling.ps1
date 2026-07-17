[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$PmkRoot,

    [string]$BackupRoot
)

$ErrorActionPreference = "Stop"
$sourceRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $sourceRoot "..\..")).Path
if ([string]::IsNullOrWhiteSpace($BackupRoot)) {
    $BackupRoot = Join-Path (Split-Path -Parent $repoRoot) "esp_backups"
}
$resolvedPmkRoot = (Resolve-Path -LiteralPath $PmkRoot).Path
$resolvedBackupRoot = (Resolve-Path -LiteralPath $BackupRoot).Path
$backupPath = Join-Path $resolvedBackupRoot (Get-Date -Format "yyyyMMdd-HHmmss")

$files = @(
    @{ Source = "ESPBlueprintAutomation\ESPBlueprintAutomation.uplugin"; Destination = "Plugins\ESPBlueprintAutomation\ESPBlueprintAutomation.uplugin" },
    @{ Source = "ESPBlueprintAutomation\Source\ESPBlueprintAutomation\ESPBlueprintAutomation.Build.cs"; Destination = "Plugins\ESPBlueprintAutomation\Source\ESPBlueprintAutomation\ESPBlueprintAutomation.Build.cs" },
    @{ Source = "ESPBlueprintAutomation\Source\ESPBlueprintAutomation\Private\ESPBlueprintAutomationLibrary.cpp"; Destination = "Plugins\ESPBlueprintAutomation\Source\ESPBlueprintAutomation\Private\ESPBlueprintAutomationLibrary.cpp" },
    @{ Source = "ESPBlueprintAutomation\Source\ESPBlueprintAutomation\Private\ESPBlueprintAutomationModule.cpp"; Destination = "Plugins\ESPBlueprintAutomation\Source\ESPBlueprintAutomation\Private\ESPBlueprintAutomationModule.cpp" },
    @{ Source = "ESPBlueprintAutomation\Source\ESPBlueprintAutomation\Public\ESPBlueprintAutomationLibrary.h"; Destination = "Plugins\ESPBlueprintAutomation\Source\ESPBlueprintAutomation\Public\ESPBlueprintAutomationLibrary.h" },
    @{ Source = "automation\create_base_assets.py"; Destination = "Automation\create_base_assets.py" },
    @{ Source = "automation\build_esp_assets.py"; Destination = "Automation\build_esp_assets.py" },
    @{ Source = "automation\create_primary_asset_label.py"; Destination = "Automation\create_primary_asset_label.py" }
)

if (Test-Path -LiteralPath $backupPath) {
    throw "Backup path already exists: $backupPath"
}

foreach ($file in $files) {
    $source = Join-Path $sourceRoot $file.Source
    $destination = Join-Path $resolvedPmkRoot $file.Destination
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Canonical source is missing: $source"
    }
    if (-not (Test-Path -LiteralPath (Split-Path -Parent $destination) -PathType Container)) {
        throw "PMK destination parent is missing: $(Split-Path -Parent $destination)"
    }
}

New-Item -ItemType Directory -Path $backupPath | Out-Null
foreach ($file in $files) {
    $source = Join-Path $sourceRoot $file.Source
    $destination = Join-Path $resolvedPmkRoot $file.Destination
    if (Test-Path -LiteralPath $destination -PathType Leaf) {
        $backupFile = Join-Path $backupPath $file.Destination
        New-Item -ItemType Directory -Path (Split-Path -Parent $backupFile) -Force | Out-Null
        Copy-Item -LiteralPath $destination -Destination $backupFile
    }
    Copy-Item -LiteralPath $source -Destination $destination -Force

    $sourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $source).Hash
    $destinationHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $destination).Hash
    if ($sourceHash -ne $destinationHash) {
        throw "Hash mismatch after sync: $($file.Destination)"
    }
}

[pscustomobject]@{
    PmkRoot = $resolvedPmkRoot
    BackupPath = $backupPath
    FilesSynced = $files.Count
}

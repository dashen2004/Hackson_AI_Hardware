$ErrorActionPreference = 'Stop'
$firmwareRoot = Join-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) 'src/firmware'
$idfTools = Join-Path $firmwareRoot 'esp-idf/tools/idf_tools.py'

if (-not (Test-Path -LiteralPath $idfTools)) {
    throw 'SDK missing. Run: git submodule update --init --recursive'
}
Get-Command git.exe, python.exe -ErrorAction Stop | Out-Null

# Use Espressif's official mirror for GitHub release assets for this process.
$previousMirror = $env:IDF_GITHUB_ASSETS
try {
    if (-not $env:IDF_GITHUB_ASSETS) {
        $env:IDF_GITHUB_ASSETS = 'dl.espressif.com/github_assets'
    }
    & python.exe $idfTools --non-interactive install --targets=esp32s3
    if ($LASTEXITCODE -ne 0) { throw 'ESP-IDF tool installation failed.' }
    & python.exe $idfTools --non-interactive install-python-env
    if ($LASTEXITCODE -ne 0) { throw 'ESP-IDF Python environment installation failed.' }
} finally {
    $env:IDF_GITHUB_ASSETS = $previousMirror
}

Write-Host 'Tools installed. Run: . ./tools/firmware/Enter-Firmware.ps1'

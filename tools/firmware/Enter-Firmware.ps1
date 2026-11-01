# Dot-source this script to activate the environment in the current shell.
$ErrorActionPreference = 'Stop'
$firmwareRoot = Join-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) 'src/firmware'
$idfRoot = Join-Path $firmwareRoot 'esp-idf'
$idfActivate = Join-Path $idfRoot 'tools/activate.py'

if (-not (Test-Path -LiteralPath $idfActivate)) {
    throw 'SDK missing. Run: git submodule update --init --recursive'
}

# Let ESP-IDF discover its Python environment and all tool versions.
$activationScript = & python.exe $idfActivate --export --shell powershell.exe
if ($LASTEXITCODE -ne 0 -or -not $activationScript) {
    throw 'ESP-IDF activation failed. Run: ./tools/firmware/Install-FirmwareTools.ps1'
}
. $activationScript

# QUMIOS CMake includes require forward slashes on Windows.
$env:ADF_PATH = $firmwareRoot.Replace('\', '/')
$env:IDF_PATH = $idfRoot.Replace('\', '/')
Write-Host "Firmware environment ready: $env:IDF_PATH"
Write-Host 'Build from the project root: idf.py -C src/firmware/apps build'

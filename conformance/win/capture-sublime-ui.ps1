param(
    [Parameter(Mandatory = $true)]
    [string]$CaptureExecutable,
    [Parameter(Mandatory = $true)]
    [string]$TestsDirectory,
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,
    [Parameter(Mandatory = $true)]
    [string]$PluginPath,
    [string]$Crop = "0,0,600,500"
)

$ErrorActionPreference = "Stop"

$sublimeProcess = Get-Process -Name sublime_text -ErrorAction SilentlyContinue |
    Where-Object { $_.MainWindowHandle -ne 0 } |
    Select-Object -First 1
if ($null -eq $sublimeProcess) {
    throw "No visible Sublime Text window was found"
}

$installDirectory = Split-Path -Parent $sublimeProcess.Path
$portableMarker = Join-Path $installDirectory "Data\KEEPME"
if (Test-Path -LiteralPath $portableMarker) {
    $userPackageDirectory = Join-Path $installDirectory "Data\Packages\User"
} else {
    $userPackageDirectory = Join-Path $env:APPDATA "Sublime Text\Packages\User"
}
New-Item -ItemType Directory -Force -Path $userPackageDirectory | Out-Null
$installedPlugin = Join-Path $userPackageDirectory "sidebar_render.py"
Copy-Item -LiteralPath $PluginPath -Destination $installedPlugin -Force
# Give a running Sublime instance time to load a newly installed command plugin.
Start-Sleep -Milliseconds 500

& $CaptureExecutable $TestsDirectory $OutputDirectory --ui --crop $Crop
exit $LASTEXITCODE

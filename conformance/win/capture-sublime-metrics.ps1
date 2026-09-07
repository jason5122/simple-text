param(
    [Parameter(Mandatory = $true)]
    [string]$TestsDirectory,
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,
    [Parameter(Mandatory = $true)]
    [string]$PluginPath,
    [int]$Limit = 0
)

$ErrorActionPreference = "Stop"

function Read-Config([string]$Path) {
    return Get-Content -LiteralPath $Path -Encoding UTF8 |
        ForEach-Object { $_.Trim() } |
        Where-Object { $_ -ne "" -and -not $_.StartsWith("#") }
}

function Wait-For-Result([string]$Path) {
    $timer = [Diagnostics.Stopwatch]::StartNew()
    while (-not (Test-Path -LiteralPath $Path)) {
        if ($timer.Elapsed.TotalSeconds -ge 20) {
            throw "Timed out waiting for $Path"
        }
        Start-Sleep -Milliseconds 20
    }

    $result = Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($null -ne $result.error) {
        throw "Metrics probe failed for ${Path}: $($result.error)"
    }
}

$sublimeProcess = Get-Process -Name sublime_text -ErrorAction SilentlyContinue |
    Where-Object { $_.MainWindowHandle -ne 0 } |
    Select-Object -First 1
if ($null -eq $sublimeProcess) {
    throw "No visible Sublime Text window was found"
}

$sublPath = Join-Path (Split-Path -Parent $sublimeProcess.Path) "subl.exe"
if (-not (Test-Path -LiteralPath $sublPath)) {
    throw "subl.exe was not found at $sublPath"
}

$installDirectory = Split-Path -Parent $sublimeProcess.Path
$portableMarker = Join-Path $installDirectory "Data\KEEPME"
if (Test-Path -LiteralPath $portableMarker) {
    $userPackageDirectory = Join-Path $installDirectory "Data\Packages\User"
} else {
    $userPackageDirectory = Join-Path $env:APPDATA "Sublime Text\Packages\User"
}
New-Item -ItemType Directory -Force -Path $userPackageDirectory | Out-Null
Copy-Item -LiteralPath $PluginPath -Destination (Join-Path $userPackageDirectory "metrics_probe.py") -Force
$requestPath = Join-Path $userPackageDirectory "metrics_probe_request.json"

if (Test-Path -LiteralPath $OutputDirectory) {
    Remove-Item -LiteralPath $OutputDirectory -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

$faces = @(Read-Config (Join-Path $TestsDirectory "faces-win.txt"))
$sizes = @(Read-Config (Join-Path $TestsDirectory "sizes.txt"))
$texts = @(Get-ChildItem -LiteralPath (Join-Path $TestsDirectory "texts") -Filter "*.txt" |
    Sort-Object Name)
if ($faces.Count -eq 0 -or $sizes.Count -eq 0 -or $texts.Count -eq 0) {
    throw "No faces, sizes, or text corpora under $TestsDirectory"
}

$total = $texts.Count * $faces.Count * $sizes.Count
$current = 0
foreach ($text in $texts) {
    foreach ($face in $faces) {
        foreach ($size in $sizes) {
            $label = "$($text.BaseName)-$face-$size"
            $outputPath = Join-Path $OutputDirectory "$label.json"
            $request = @{
                text_path = $text.FullName
                face = $face
                size = [double]::Parse($size, [Globalization.CultureInfo]::InvariantCulture)
                output_path = $outputPath
            } | ConvertTo-Json -Compress
            [IO.File]::WriteAllText($requestPath, $request, [Text.UTF8Encoding]::new($false))
            & $sublPath --background --command metrics_probe | Out-Null
            Wait-For-Result $outputPath

            $current += 1
            Write-Output "[$current/$total] $outputPath"
            if ($Limit -gt 0 -and $current -ge $Limit) {
                return
            }
        }
    }
}

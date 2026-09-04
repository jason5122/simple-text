param(
    [Parameter(Mandatory = $true)]
    [string]$TestsDirectory,
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,
    [Parameter(Mandatory = $true)]
    [string]$ExecutablePath,
    [int]$Limit = 0
)

$ErrorActionPreference = "Stop"

function Read-Config([string]$Path) {
    return @(
        Get-Content -LiteralPath $Path |
            ForEach-Object { $_.Trim() } |
            Where-Object { $_ -ne "" -and -not $_.StartsWith("#") }
    )
}

$faceFiles = @(Get-ChildItem -LiteralPath $TestsDirectory -Filter "faces-*.txt")
$textFiles = @(Get-ChildItem -LiteralPath (Join-Path $TestsDirectory "texts") -Filter "*.txt" |
    Sort-Object Name)
if ($faceFiles.Count -ne 1 -or $textFiles.Count -eq 0) {
    throw "Expected one faces file and at least one text corpus under $TestsDirectory"
}

$faces = @(Read-Config $faceFiles[0].FullName)
$sizes = @(Read-Config (Join-Path $TestsDirectory "sizes.txt"))
if ($faces.Count -eq 0 -or $sizes.Count -eq 0) {
    throw "No faces or sizes under $TestsDirectory"
}

if (Test-Path -LiteralPath $OutputDirectory) {
    Remove-Item -LiteralPath $OutputDirectory -Recurse -Force
}
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null

$total = $textFiles.Count * $faces.Count * $sizes.Count
$current = 0
foreach ($textFile in $textFiles) {
    foreach ($face in $faces) {
        foreach ($size in $sizes) {
            $outputName = "{0}-{1}-{2}.json" -f $textFile.BaseName, $face, $size
            $outputPath = Join-Path $OutputDirectory $outputName
            & $ExecutablePath $textFile.FullName $face $size $outputPath
            if ($LASTEXITCODE -ne 0) {
                throw "metrics_conformance failed for $outputName with exit code $LASTEXITCODE"
            }

            $current += 1
            Write-Host "[$current/$total] $outputPath"
            if ($Limit -gt 0 -and $current -ge $Limit) {
                return
            }
        }
    }
}

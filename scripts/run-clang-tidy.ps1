#Requires -Version 5.1
# run-clang-tidy.ps1
# Usage (from project root):
#   .\scripts\run-clang-tidy.ps1          # check only
#   .\scripts\run-clang-tidy.ps1 -Fix     # auto-fix where possible

param(
    [switch]$Fix,
    [string]$BuildDir = "build"
)

Set-Location (Split-Path $PSScriptRoot -Parent)

function Write-Banner([string]$text) {
    $sep = "-" * 62
    Write-Host ""
    Write-Host $sep                    -ForegroundColor Cyan
    Write-Host ("  " + $text)         -ForegroundColor Cyan
    Write-Host $sep                    -ForegroundColor Cyan
}

# --- Find clang-tidy -------------------------------------------------
$ct = Get-Command clang-tidy -ErrorAction SilentlyContinue
if (-not $ct) {
    Write-Host "[ERROR] clang-tidy not found." -ForegroundColor Red
    Write-Host "        Install: winget install LLVM.LLVM  then restart terminal" -ForegroundColor Yellow
    exit 1
}

# --- Ensure compile_commands.json ------------------------------------
$dbPath = Join-Path $BuildDir "compile_commands.json"
if (-not (Test-Path $dbPath)) {
    Write-Host "compile_commands.json missing - running cmake..." -ForegroundColor Yellow
    if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory -Force $BuildDir | Out-Null }
    cmake -B $BuildDir -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTS=OFF
    if ($LASTEXITCODE -ne 0) { Write-Host "[ERROR] cmake failed." -ForegroundColor Red; exit 1 }
}

# --- Detect MinGW includes -------------------------------------------
$gpp    = Get-Command g++ -ErrorAction SilentlyContinue
$mingw  = if ($gpp) { Split-Path (Split-Path $gpp.Source) } else { "C:\msys64\ucrt64" }
$cppVer = (Get-ChildItem "$mingw\include\c++" -Directory -ErrorAction SilentlyContinue |
           Sort-Object Name -Descending | Select-Object -First 1).Name

$extra = @(
    "--extra-arg=--target=x86_64-w64-mingw32"
    "--extra-arg=-isystem"; "--extra-arg=$mingw/include/c++/$cppVer"
    "--extra-arg=-isystem"; "--extra-arg=$mingw/include/c++/$cppVer/x86_64-w64-mingw32"
    "--extra-arg=-isystem"; "--extra-arg=$mingw/include"
)
if ($Fix) { $extra += "--fix"; $extra += "--fix-errors" }

# --- Collect driver files (cpp) --------------------------------------
$sources = @(Get-ChildItem -Path src -Filter *.cpp -Recurse)

# --- Print header ----------------------------------------------------
Write-Banner "clang-tidy check -- Event System"
Write-Host ("  Tool    : " + $ct.Source)  -ForegroundColor Gray
Write-Host ("  Mode    : " + $(if ($Fix) { "AUTO-FIX" } else { "check only" })) `
           -ForegroundColor $(if ($Fix) { "Yellow" } else { "Gray" })
Write-Host ("  Sources : " + $sources.Count + " .cpp files (+ all included headers)") -ForegroundColor Gray

# --- Run all cpp files and collect ALL raw output --------------------
Write-Host ""
Write-Host "  Running clang-tidy..." -ForegroundColor DarkCyan

$allLines = @()
foreach ($f in $sources) {
    $raw = & clang-tidy $f.FullName -p $BuildDir --config-file=.clang-tidy @extra 2>&1
    $allLines += $raw
}

# --- Parse: group diagnostic lines by the FILE they come from --------
# A diagnostic block looks like:
#   path/to/file.hpp:15:9: warning: message [check-name]
#     code snippet
#     ^~~~
# We group by the leading file path.

$projectRoot = (Get-Location).Path.Replace('\', '/')

# Only keep lines that originate from our project (not system headers)
$diagLines = $allLines | Where-Object {
    ($_ -match "^[A-Za-z]:[/\\].+\.(cpp|hpp):\d+:\d+: (warning|error):") -and
    ($_ -match [regex]::Escape($projectRoot)) -and
    ($_ -notmatch "clang-tidy-config")
}

# Group by filename (basename)
$byFile = @{}
foreach ($line in $diagLines) {
    if ($line -match "^.+[/\\]([^/\\]+\.(cpp|hpp)):\d+:\d+: (warning|error):") {
        $fname = $Matches[1]
        if (-not $byFile.ContainsKey($fname)) { $byFile[$fname] = @() }
        $byFile[$fname] += $line
    }
}

# --- Separate .hpp and .cpp results ----------------------------------
$cppFiles    = $sources | ForEach-Object { $_.Name }
$allChecked  = ($sources | ForEach-Object { $_.Name }) +
               (Get-ChildItem -Path include -Filter *.hpp -Recurse | ForEach-Object { $_.Name } |
                Select-Object -Unique)

# Count totals
$warnTotal = ($diagLines | Where-Object { $_ -match ": warning:" }).Count
$errTotal  = ($diagLines | Where-Object { $_ -match ": error:"   }).Count

# --- Results: cpp files ----------------------------------------------
Write-Host ""
Write-Host "  Source files (.cpp):" -ForegroundColor DarkCyan
foreach ($f in $sources) {
    if ($byFile.ContainsKey($f.Name)) {
        $e = ($byFile[$f.Name] | Where-Object { $_ -match ": error:"   }).Count
        $w = ($byFile[$f.Name] | Where-Object { $_ -match ": warning:" }).Count
        Write-Host ("  [FAIL] " + $f.Name + "  (" + $e + " err, " + $w + " warn)") -ForegroundColor Red
    } else {
        Write-Host ("  [ OK ] " + $f.Name) -ForegroundColor Green
    }
}

# --- Results: header files -------------------------------------------
$hppFiles = Get-ChildItem -Path include -Filter *.hpp -Recurse
Write-Host ""
Write-Host "  Header files (.hpp):" -ForegroundColor DarkCyan
foreach ($h in $hppFiles) {
    if ($byFile.ContainsKey($h.Name)) {
        $e = ($byFile[$h.Name] | Where-Object { $_ -match ": error:"   }).Count
        $w = ($byFile[$h.Name] | Where-Object { $_ -match ": warning:" }).Count
        Write-Host ("  [FAIL] " + $h.Name + "  (" + $e + " err, " + $w + " warn)") -ForegroundColor Red
    } else {
        Write-Host ("  [ OK ] " + $h.Name) -ForegroundColor Green
    }
}

# --- Detailed issues -------------------------------------------------
if ($byFile.Count -gt 0) {
    Write-Host ""
    Write-Host "  Detailed issues:" -ForegroundColor DarkCyan
    foreach ($entry in ($byFile.GetEnumerator() | Sort-Object Key)) {
        Write-Host ""
        Write-Host ("  >> " + $entry.Key) -ForegroundColor Yellow
        foreach ($line in $entry.Value) {
            if     ($line -match ": error:")   { Write-Host ("     " + $line) -ForegroundColor Red    }
            elseif ($line -match ": warning:") { Write-Host ("     " + $line) -ForegroundColor Yellow }
        }
    }
}

# --- Summary ---------------------------------------------------------
$totalFiles  = $sources.Count + ($hppFiles | Measure-Object).Count
$failedFiles = $byFile.Count
$okFiles     = $totalFiles - $failedFiles

Write-Host ""
Write-Host "  ----------------------------------------" -ForegroundColor DarkGray
Write-Host ("  Total files checked : " + $totalFiles + "  (" + $sources.Count + " cpp + " + ($hppFiles | Measure-Object).Count + " hpp)")
Write-Host ("  Files OK            : " + $okFiles)     -ForegroundColor $(if ($okFiles -eq $totalFiles) { "Green" } else { "White" })
Write-Host ("  Files with issues   : " + $failedFiles) -ForegroundColor $(if ($failedFiles -gt 0) { "Red" } else { "Green" })
Write-Host ("  Warnings            : " + $warnTotal)   -ForegroundColor $(if ($warnTotal -gt 0) { "Yellow" } else { "Green" })
Write-Host ("  Errors              : " + $errTotal)    -ForegroundColor $(if ($errTotal  -gt 0) { "Red"    } else { "Green" })
Write-Host "  ----------------------------------------" -ForegroundColor DarkGray

if ($errTotal -eq 0 -and $warnTotal -eq 0) {
    Write-Host "  [PASS] All checks passed." -ForegroundColor Green
} elseif ($Fix) {
    Write-Host "  [DONE] Auto-fix applied. Re-run without -Fix to verify." -ForegroundColor Yellow
} else {
    Write-Host "  [FAIL] Issues found. Re-run with -Fix to auto-fix." -ForegroundColor Red
}
Write-Host ""

exit $(if ($errTotal -gt 0) { 1 } else { 0 })

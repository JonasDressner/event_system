# ── run-clang-format.ps1 ────────────────────────────────────────────
# Checks or applies clang-format on all project source files.
# Prerequisites: LLVM installed (winget install LLVM.LLVM)
#
# Usage (from project root):
#   .\scripts\run-clang-format.ps1          # check only (exit 1 if unformatted)
#   .\scripts\run-clang-format.ps1 -Fix     # format in-place

param(
    [switch]$Fix
)

$ErrorActionPreference = "Stop"

# ── Find clang-format ────────────────────────────────────────────────
$clangFormat = Get-Command clang-format -ErrorAction SilentlyContinue
if (-not $clangFormat) {
    Write-Error "clang-format not found. Install via: winget install LLVM.LLVM"
    exit 1
}
Write-Host "Using: $($clangFormat.Source)" -ForegroundColor Cyan

# ── Collect files ─────────────────────────────────────────────────────
$files = @(
    Get-ChildItem -Path src     -Filter *.cpp -Recurse
    Get-ChildItem -Path include -Filter *.hpp -Recurse
    Get-ChildItem -Path test    -Filter *.cpp -Recurse
    Get-ChildItem -Path test    -Filter *.hpp -Recurse
)

Write-Host "Processing $($files.Count) files..." -ForegroundColor Cyan

if ($Fix) {
    Write-Host "Mode: FORMAT in-place" -ForegroundColor Yellow
    foreach ($file in $files) {
        & clang-format -i --style=file $file.FullName
        Write-Host "  formatted  $($file.Name)" -ForegroundColor Green
    }
    Write-Host "`nAll files formatted." -ForegroundColor Green
} else {
    Write-Host "Mode: CHECK only (use -Fix to format)" -ForegroundColor Gray
    $unformatted = @()
    foreach ($file in $files) {
        $original  = Get-Content $file.FullName -Raw
        $formatted = & clang-format --style=file $file.FullName
        if ($original -ne ($formatted -join "`n")) {
            $unformatted += $file.Name
            Write-Host "  DIFF  $($file.Name)" -ForegroundColor Red
        } else {
            Write-Host "  OK    $($file.Name)" -ForegroundColor Green
        }
    }

    Write-Host ""
    if ($unformatted.Count -gt 0) {
        Write-Host "$($unformatted.Count) file(s) need formatting. Run with -Fix to apply." -ForegroundColor Red
        exit 1
    } else {
        Write-Host "All files correctly formatted." -ForegroundColor Green
    }
}

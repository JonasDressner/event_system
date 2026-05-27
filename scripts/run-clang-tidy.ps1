# ── run-clang-tidy.ps1 ──────────────────────────────────────────────
# Runs clang-tidy on all project source files using compile_commands.json.
# Prerequisites:
#   1. LLVM installed:  winget install LLVM.LLVM
#   2. Build configured: cmake -B build  (generates compile_commands.json)
#
# Usage (from project root):
#   .\scripts\run-clang-tidy.ps1          # check only
#   .\scripts\run-clang-tidy.ps1 -Fix     # auto-fix where possible

param(
    [switch]$Fix,
    [string]$BuildDir = "build"
)

Set-Location (Split-Path $PSScriptRoot -Parent)
$ErrorActionPreference = "Stop"

# ── Find clang-tidy ─────────────────────────────────────────────────
$clangTidyExe = Get-Command clang-tidy -ErrorAction SilentlyContinue
if (-not $clangTidyExe) {
    Write-Error "clang-tidy not found. Install via: winget install LLVM.LLVM (then restart terminal)"
    exit 1
}
Write-Host "Using: $($clangTidyExe.Source)" -ForegroundColor Cyan

# ── Check compile_commands.json ──────────────────────────────────────
$compileDb = Join-Path $BuildDir "compile_commands.json"
if (-not (Test-Path $compileDb)) {
    Write-Host "compile_commands.json not found. Running cmake..." -ForegroundColor Yellow
    if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory -Force $BuildDir | Out-Null }
    cmake -B $BuildDir -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    if ($LASTEXITCODE -ne 0) { Write-Error "cmake configure failed"; exit 1 }
}

# ── Collect source files ─────────────────────────────────────────────
$sources = Get-ChildItem -Path src -Filter *.cpp -Recurse
Write-Host "Checking $($sources.Count) source files..." -ForegroundColor Cyan

# ── Build clang-tidy arguments ────────────────────────────────────────
$baseArgs = @("-p", $BuildDir, "--config-file=.clang-tidy")
if ($Fix) {
    $baseArgs += @("--fix", "--fix-errors")
    Write-Host "Mode: AUTO-FIX enabled" -ForegroundColor Yellow
} else {
    Write-Host "Mode: check only  (use -Fix to apply fixes)" -ForegroundColor Gray
}

# ── Run ──────────────────────────────────────────────────────────────
$hasErrors = $false
foreach ($file in $sources) {
    $output = & clang-tidy $file.FullName @baseArgs 2>&1
    if ($LASTEXITCODE -ne 0) {
        $hasErrors = $true
        Write-Host ""
        Write-Host "[$($file.Name)]" -ForegroundColor Red
        $output | Where-Object { $_ -match "warning:|error:" } | Write-Host
    } else {
        Write-Host "  OK   $($file.Name)" -ForegroundColor Green
    }
}

Write-Host ""
if ($hasErrors) {
    Write-Host "clang-tidy found issues. Fix manually or re-run with -Fix." -ForegroundColor Red
    exit 1
} else {
    Write-Host "All checks passed." -ForegroundColor Green
}

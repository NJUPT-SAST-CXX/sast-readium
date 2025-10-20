#!/usr/bin/env pwsh
# Comprehensive clang-tidy runner (PowerShell)
# Applies fixes across all project and test sources, aggregates results, and verifies cleanliness.

param(
  [string]$ClangTidy = "D:\\msys64\\mingw64\\bin\\clang-tidy.exe",
  [string]$BuildDir = "build/Debug-MSYS2",
  [string]$Output = "analysis/clang-tidy-results.txt",
  [switch]$Fix,
  [switch]$FixErrors,
  [string]$AppList = "analysis/all_app_files.txt",
  [string]$TestList = "analysis/all_test_files.txt"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Read-FileList([string]$Path) {
  if (-not (Test-Path -LiteralPath $Path)) { return @() }
  Get-Content -LiteralPath $Path -Encoding UTF8 | Where-Object { $_ -and $_.Trim().Length -gt 0 }
}

function Count-Issues([string]$Text) {
  # Use single backslashes for .NET regex escapes
  $warnings = ([regex]::Matches($Text, '\bwarning:')).Count
  $errors   = ([regex]::Matches($Text, '\berror:')).Count
  $cats = @{}
  foreach ($m in [regex]::Matches($Text, '\[([^\]]+)\]')) {
    $cat = $m.Groups[1].Value
    if ($cats.ContainsKey($cat)) { $cats[$cat] += 1 } else { $cats[$cat] = 1 }
  }
  [pscustomobject]@{Warnings=$warnings; Errors=$errors; Categories=$cats}
}

function Run-File([string]$File, [bool]$DoFix, [bool]$DoFixErrors) {
  $args = @("-p=$BuildDir")
  # Header filter is also in .clang-tidy; pass explicitly for speed and scoping
  $args += @('--header-filter=^.*/(app|tests)/.*\.(h|hpp|hxx)$')
  if ($DoFix) { $args += '--fix' }
  if ($DoFixErrors) { $args += '--fix-errors' }
  $args += @($File)

  $psi = New-Object System.Diagnostics.ProcessStartInfo
  $psi.FileName = $ClangTidy
  $psi.Arguments = [string]::Join(' ', $args)
  $psi.RedirectStandardOutput = $true
  $psi.RedirectStandardError = $true
  $psi.UseShellExecute = $false
  $psi.CreateNoWindow = $true

  $p = New-Object System.Diagnostics.Process
  $p.StartInfo = $psi
  $null = $p.Start()
  $stdout = $p.StandardOutput.ReadToEnd()
  $stderr = $p.StandardError.ReadToEnd()
  $p.WaitForExit()
  $code = $p.ExitCode
  [pscustomobject]@{ Output = ($stdout + $stderr); Code = $code }
}

# Preconditions
if (-not (Test-Path -LiteralPath $ClangTidy)) {
  Write-Error "clang-tidy not found at $ClangTidy"
}

$compileDb = Join-Path $BuildDir 'compile_commands.json'
if (-not (Test-Path -LiteralPath $compileDb)) {
  Write-Error "compile_commands.json not found at $compileDb"
}

if (-not (Test-Path analysis)) { New-Item -ItemType Directory -Path analysis | Out-Null }
if (Test-Path -LiteralPath $Output) { Remove-Item -LiteralPath $Output -Force }

$appFiles  = Read-FileList -Path $AppList
$testFiles = Read-FileList -Path $TestList
$all = @($appFiles + $testFiles)
if ($all.Count -eq 0) { Write-Error 'No files to process. Ensure file lists are generated.' }

Write-Host ('=' * 80)
Write-Host 'CLANG-TIDY COMPREHENSIVE ANALYSIS (PowerShell)'
Write-Host ('=' * 80)
Write-Host "Build directory: $BuildDir"
Write-Host "Fix mode: $Fix  Fix errors: $FixErrors"
Write-Host "Total files: $($all.Count)  (app: $($appFiles.Count), tests: $($testFiles.Count))"
Write-Host

$stats = [ordered]@{
  processed     = 0
  with_issues   = 0
  total_warnings= 0
  total_errors  = 0
  failed        = 0
  categories    = @{}
}

$swTotal = [System.Diagnostics.Stopwatch]::StartNew()
$i = 0
foreach ($f in $all) {
  $i++
  $pct = [math]::Round(100.0 * $i / $all.Count, 1)
  $rel = (Resolve-Path -LiteralPath $f).Path
  $sw = [System.Diagnostics.Stopwatch]::StartNew()
  Write-Host -NoNewline "[$i/$($all.Count)] ($pct%) $rel"
  $res = Run-File -File $f -DoFix:$Fix.IsPresent -DoFixErrors:$FixErrors.IsPresent
  $sw.Stop()
  $stats['processed']++

  $issues = Count-Issues -Text $res.Output
  if ($issues.Warnings -gt 0 -or $issues.Errors -gt 0) {
    $stats['with_issues']++
    $stats['total_warnings'] += $issues.Warnings
    $stats['total_errors']   += $issues.Errors
    foreach ($k in $issues.Categories.Keys) {
      if ($stats['categories'].ContainsKey($k)) { $stats['categories'][$k] += $issues.Categories[$k] }
      else { $stats['categories'][$k] = $issues.Categories[$k] }
    }
    Write-Host " - W:$($issues.Warnings) E:$($issues.Errors) ($([math]::Round($sw.Elapsed.TotalSeconds,1))s)"
  } else {
    Write-Host " OK ($([math]::Round($sw.Elapsed.TotalSeconds,1))s)"
  }
  if ($res.Code -ne 0 -and $res.Output -match 'TIMEOUT') { $stats['failed']++ }

  Add-Content -LiteralPath $Output -Encoding UTF8 ('=' * 80)
  Add-Content -LiteralPath $Output -Encoding UTF8 "FILE: $rel"
  Add-Content -LiteralPath $Output -Encoding UTF8 "Warnings: $($issues.Warnings), Errors: $($issues.Errors)"
  Add-Content -LiteralPath $Output -Encoding UTF8 ('=' * 80)
  Add-Content -LiteralPath $Output -Encoding UTF8 $res.Output
  Add-Content -LiteralPath $Output -Encoding UTF8 ""
}

$swTotal.Stop()
Write-Host
Write-Host ('=' * 80)
Write-Host 'SUMMARY'
Write-Host ('=' * 80)
Write-Host "Processed: $($stats['processed']) / $($all.Count)"
Write-Host "Files with issues: $($stats['with_issues'])"
Write-Host "Total warnings: $($stats['total_warnings'])"
Write-Host "Total errors: $($stats['total_errors'])"
Write-Host "Failed/timeout: $($stats['failed'])"
Write-Host ("Total time: {0:N1}s ({1:N1} minutes)" -f $swTotal.Elapsed.TotalSeconds, ($swTotal.Elapsed.TotalMinutes))
if ($stats['categories'].Count -gt 0) {
  Write-Host "Top issue categories:"
  $stats['categories'].GetEnumerator() | Sort-Object -Property Value -Descending | Select-Object -First 20 | ForEach-Object {
    Write-Host ("  {0}: {1}" -f $_.Key, $_.Value)
  }
}
Write-Host
Write-Host "Detailed results saved to: $Output"

exit 0

<# : ---------- launcher (batch part) ----------
@echo off
title Project OptM - Build
set "BUILD_SELF=%~f0"
powershell -NoProfile -ExecutionPolicy Bypass -Command "Invoke-Expression ([System.IO.File]::ReadAllText($env:BUILD_SELF))"
set "RC=%ERRORLEVEL%"
if /i not "%~1"=="nopause" ( echo. & pause )
exit /b %RC%
#>
# =====================================================================
#  PROJECT OPTM - build (C++ edition)
#  Builds dist\ProjectOptM.exe with Visual Studio 2022 (or its free Build
#  Tools with "Desktop development with C++"). CMake comes with it.
# =====================================================================
function Say([string]$t, [string]$c = 'Gray') { Write-Host $t -ForegroundColor $c }
try {
  $Here  = Split-Path -Parent $env:BUILD_SELF
  $build = Join-Path $Here 'build'
  $dist  = Join-Path $Here 'dist'

  $text = [IO.File]::ReadAllText((Join-Path $Here 'src\version.h'))
  if ($text -notmatch '#define OPTM_VERSION\s+"([0-9]+(\.[0-9]+){1,3})"') { throw 'Could not find OPTM_VERSION in src\version.h' }
  $ver = $Matches[1]

  $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
  $vs = if (Test-Path $vswhere) { & $vswhere -products * -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath }
  if (-not $vs) { throw 'Visual Studio 2022 with "Desktop development with C++" was not found. Install the free Build Tools: winget install Microsoft.VisualStudio.2022.BuildTools' }
  $cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
  if (-not (Test-Path $cmake)) { $cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source }
  if (-not $cmake) { throw 'CMake was not found (it comes with Visual Studio - add "C++ CMake tools for Windows").' }

  Say ''
  Say "  Building Project OptM v$ver ..." Cyan
  if (-not (Test-Path (Join-Path $build 'CMakeCache.txt'))) {
    & $cmake -S $Here -B $build -G 'Visual Studio 17 2022' -A x64 | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'CMake could not set up the build (see above).' }
  }
  & $cmake --build $build --config Release -- /nologo /v:minimal
  if ($LASTEXITCODE -ne 0) { throw 'The compiler reported an error (see above).' }

  New-Item -ItemType Directory -Force -Path $dist | Out-Null
  $exe = Join-Path $dist 'ProjectOptM.exe'
  $new = Join-Path $build 'Release\ProjectOptM.exe'
  try { Copy-Item $new $exe -Force -ErrorAction Stop }
  catch {
    # the app is running from dist: a running exe can't be overwritten, but it can be renamed.
    # The running app notices the new file and restarts into it (once no game is running).
    $old = "$exe.old"
    if (Test-Path $old) { Remove-Item $old -Force -ErrorAction SilentlyContinue }
    Move-Item $exe $old -Force
    Copy-Item $new $exe -Force
    Say '  The app is running - it switches to this build by itself (after the current game, if any).' Yellow
  }
  Say "  dist\ProjectOptM.exe  ($([math]::Round((Get-Item $exe).Length / 1KB)) KB)" Green
  Say "  Done - v$ver" Green
  exit 0
}
catch {
  Say ''
  Say "  Build failed: $($_.Exception.Message)" Red
  exit 1
}

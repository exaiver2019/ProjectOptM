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
#  PROJECT OPTM - build
#  Packs src\ProjectOptM.ps1 into dist\ProjectOptM.exe (a real Windows app
#  with its own icon and admin prompt) and dist\ProjectOptM.bat (for older
#  installs). Uses the C# compiler built into Windows - nothing to install.
# =====================================================================
function Say([string]$t, [string]$c = 'Gray') { Write-Host $t -ForegroundColor $c }
try {
  $Here = Split-Path -Parent $env:BUILD_SELF
  $src  = Join-Path $Here 'src'
  $dist = Join-Path $Here 'dist'
  $ps1  = Join-Path $src 'ProjectOptM.ps1'
  New-Item -ItemType Directory -Force -Path $dist | Out-Null

  $text = [IO.File]::ReadAllText($ps1)
  if ($text -notmatch "\`$AppVersion\s*=\s*'([0-9]+(\.[0-9]+){1,3})'") { throw 'Could not find $AppVersion in src\ProjectOptM.ps1' }
  $ver = $Matches[1]
  $ver4 = (@($ver.Split('.')) + @('0', '0', '0'))[0..3] -join '.'

  # make sure the script itself is valid before packing it
  $errs = $null
  [void][System.Management.Automation.Language.Parser]::ParseInput($text, [ref]$null, [ref]$errs)
  if ($errs) { throw "src\ProjectOptM.ps1 has a syntax error: $($errs[0].Message) (line $($errs[0].Extent.StartLineNumber))" }

  $csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
  if (-not (Test-Path $csc)) { $csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe' }
  if (-not (Test-Path $csc)) { throw "The .NET Framework C# compiler wasn't found ($csc)." }

  $info = Join-Path $env:TEMP 'optm-assemblyinfo.cs'
  [IO.File]::WriteAllText($info, @"
using System.Reflection;
[assembly: AssemblyTitle("Project OptM")]
[assembly: AssemblyDescription("Per-game performance optimizer")]
[assembly: AssemblyProduct("Project OptM")]
[assembly: AssemblyCompany("exaiver2019")]
[assembly: AssemblyCopyright("exaiver2019")]
[assembly: AssemblyVersion("$ver4")]
[assembly: AssemblyFileVersion("$ver4")]
[assembly: AssemblyInformationalVersion("$ver")]
"@)

  $exe = Join-Path $dist 'ProjectOptM.exe'
  Say ''
  Say "  Building Project OptM v$ver ..." Cyan
  & $csc /nologo /target:winexe /optimize+ /platform:anycpu "/out:$exe" "/win32icon:$(Join-Path $src 'ProjectOptM.ico')" "/win32manifest:$(Join-Path $src 'app.manifest')" "/resource:$ps1,ProjectOptM.ps1" /reference:System.Windows.Forms.dll (Join-Path $src 'Launcher.cs') $info
  if ($LASTEXITCODE -ne 0) { throw 'The compiler reported an error (see above).' }
  Remove-Item $info -ErrorAction SilentlyContinue

  # classic .bat build, for people still on the .bat version
  $header = [IO.File]::ReadAllText((Join-Path $src 'batch-header.txt'))
  [IO.File]::WriteAllText((Join-Path $dist 'ProjectOptM.bat'), $header + $text)

  Say "  dist\ProjectOptM.exe  ($([math]::Round((Get-Item $exe).Length / 1KB)) KB)" Green
  Say "  dist\ProjectOptM.bat" Green
  Say "  Done - v$ver" Green
  exit 0
}
catch {
  Say ''
  Say "  Build failed: $($_.Exception.Message)" Red
  exit 1
}

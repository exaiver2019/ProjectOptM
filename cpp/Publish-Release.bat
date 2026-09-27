<# : ---------- launcher (batch part) ----------
@echo off
title Project OptM - Publish release
set "REL_SELF=%~f0"
powershell -NoProfile -ExecutionPolicy Bypass -Command "Invoke-Expression ([System.IO.File]::ReadAllText($env:REL_SELF))"
echo.
pause
exit /b
#>
# =====================================================================
#  PROJECT OPTM - one-click release publisher (C++ edition)
#
#  Double-click it from this folder. It will:
#   1. read the version from src\version.h (OPTM_VERSION)
#   2. build dist\ProjectOptM.exe with Build.bat
#   3. ask what changed
#   4. publish a GitHub release (tag v<version>) with ProjectOptM.exe attached
#
#  The source and the front page (README.md) go up with git - push before
#  publishing, so the release tag points at the code it was built from.
#
#  Everyone on 1.1 or newer (the .exe) gets the update from inside the app.
#  First run only: installs GitHub CLI and asks you to sign in.
# =====================================================================

$Repo = 'exaiver2019/ProjectOptM'

$ErrorActionPreference = 'Continue'
function Say([string]$t, [string]$c = 'Gray') { Write-Host $t -ForegroundColor $c }

try {
  $Here = Split-Path -Parent $env:REL_SELF
  $Ver  = Join-Path $Here 'src\version.h'
  $Exe  = Join-Path $Here 'dist\ProjectOptM.exe'
  Say ''
  Say '  PROJECT OPTM  -  Publish a release' Cyan
  Say '  ----------------------------------' DarkGray

  # --- GitHub CLI ---
  $gh = (Get-Command gh -ErrorAction SilentlyContinue).Source
  $ghDefault = Join-Path $env:ProgramFiles 'GitHub CLI\gh.exe'
  if (-not $gh -and (Test-Path $ghDefault)) { $gh = $ghDefault }
  if (-not $gh) {
    Say '  GitHub CLI is not installed - installing it now (one time)...' Yellow
    winget install --id GitHub.cli -e --accept-source-agreements --accept-package-agreements
    if (Test-Path $ghDefault) { $gh = $ghDefault }
    else { throw 'GitHub CLI install finished but gh.exe was not found. Close this window and run it again.' }
  }

  # --- Sign in (one time) ---
  & $gh auth status *> $null
  if ($LASTEXITCODE -ne 0) {
    Say '  Sign in to GitHub (one time). Follow the steps below - a browser will open.' Yellow
    & $gh auth login --hostname github.com --git-protocol https --web
    if ($LASTEXITCODE -ne 0) { throw 'GitHub sign-in did not finish.' }
  }

  # --- Version from src\version.h ---
  $text = [IO.File]::ReadAllText($Ver)
  if ($text -notmatch '#define OPTM_VERSION\s+"([0-9]+(\.[0-9]+){1,3})"') { throw 'Could not find OPTM_VERSION in src\version.h.' }
  $ver = $Matches[1]; $tag = "v$ver"
  if ($text -notmatch [regex]::Escape("#define OPTM_UPDATE_REPO   `"$Repo`"")) { Say "  Warning: OPTM_UPDATE_REPO in src\version.h is not '$Repo' - users won't get updates." Yellow }
  $rcParts = (@($ver.Split('.')) + @('0', '0', '0'))[0..3] -join ',\s*'
  if ($text -notmatch "OPTM_VERSION_RC\s+$rcParts\b") { Say '  Warning: OPTM_VERSION_RC in src\version.h does not match OPTM_VERSION.' Yellow }

  & $gh release view $tag --repo $Repo *> $null
  if ($LASTEXITCODE -eq 0) { throw "$tag is already released. Raise OPTM_VERSION in src\version.h first (e.g. $ver -> next number)." }
  $latest = "$(& $gh release view --repo $Repo --json tagName --jq .tagName 2>$null)".Trim()
  if ($latest) {
    Say "  Latest on GitHub:  $latest"
    try { if ([version]$latest.TrimStart('v', 'V') -ge [version]$ver) { throw "The new version ($ver) must be higher than $latest." } } catch [System.Management.Automation.RuntimeException] { throw }
  }
  Say "  Publishing:        $tag" Green

  # --- Build ---
  & cmd.exe /c "`"$(Join-Path $Here 'Build.bat')`" nopause"
  if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Exe)) { throw 'The build failed - nothing was published.' }
  $fv = (Get-Item $Exe).VersionInfo.ProductVersion
  if ($fv -ne $ver) { throw "dist\ProjectOptM.exe reports version '$fv', expected '$ver' - nothing was published." }
  Say ''

  # --- Release notes ---
  Say '  What changed? One item per line. Press Enter on an empty line when done.' Cyan
  $items = @()
  while ($true) {
    $l = Read-Host '   -'
    if ([string]::IsNullOrWhiteSpace($l)) { break }
    $items += "- $($l.Trim())"
  }
  if ($items.Count -eq 0) { $items = @('- Improvements and fixes') }
  $notes = "## Project OptM $tag`n`n### What's new`n" + ($items -join "`n")
  $notesFile = Join-Path $env:TEMP 'optm-release-notes.md'
  [IO.File]::WriteAllText($notesFile, $notes)

  # --- Confirm ---
  Say ''
  Say '  ----------------------------------' DarkGray
  $notes -split "`n" | ForEach-Object { Say "  $_" }
  Say '  ----------------------------------' DarkGray
  Say "  File: ProjectOptM.exe ($([math]::Round((Get-Item $Exe).Length / 1KB)) KB)"
  Say ''
  $ok = Read-Host "  Publish $tag to github.com/$Repo ? (y/n)"
  if ($ok -notmatch '^[yY]') { Say '  Cancelled - nothing was published.' Yellow; return }

  # --- Publish ---
  & $gh release create $tag $Exe --repo $Repo --title "Project OptM $tag" --notes-file $notesFile
  if ($LASTEXITCODE -ne 0) { throw 'Publishing the release failed (see the message above).' }

  Say ''
  Say "  Done! $tag is live: https://github.com/$Repo/releases/latest" Green
  Say '  Everyone running Project OptM will be offered the update within a few hours.' Gray
}
catch {
  Say ''
  Say "  $($_.Exception.Message)" Red
}

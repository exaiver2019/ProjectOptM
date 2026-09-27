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
#  PROJECT OPTM - one-click release publisher
#
#  Double-click it from the ProjectOptM-dev folder. It will:
#   1. read the version from src\ProjectOptM.ps1 ($AppVersion)
#   2. build dist\ProjectOptM.exe (and ProjectOptM.bat) with Build.bat
#   3. ask what changed
#   4. publish a GitHub release (tag v<version>) with both files attached
#   5. upload anything in the "repo" folder (README, banner, icon) to the
#      repo's front page - only files that actually changed
#
#  First run only: installs GitHub CLI and asks you to sign in.
# =====================================================================

$Repo = 'exaiver2019/ProjectOptM'

$ErrorActionPreference = 'Continue'
function Say([string]$t, [string]$c = 'Gray') { Write-Host $t -ForegroundColor $c }

try {
  $Here = Split-Path -Parent $env:REL_SELF
  $App  = Join-Path $Here 'src\ProjectOptM.ps1'
  $Exe  = Join-Path $Here 'dist\ProjectOptM.exe'
  $Bat  = Join-Path $Here 'dist\ProjectOptM.bat'
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

  # --- Version from src\ProjectOptM.ps1 ---
  if (-not (Test-Path -LiteralPath $App)) { throw "src\ProjectOptM.ps1 was not found ($Here)." }
  $text = [IO.File]::ReadAllText($App)
  if ($text -notmatch "\`$AppVersion\s*=\s*'([0-9]+(\.[0-9]+){1,3})'") { throw 'Could not find $AppVersion in src\ProjectOptM.ps1.' }
  $ver = $Matches[1]; $tag = "v$ver"
  if ($text -notmatch [regex]::Escape("`$UpdateRepo = '$Repo'")) { Say "  Warning: `$UpdateRepo in src\ProjectOptM.ps1 is not '$Repo' - users won't get updates." Yellow }

  & $gh release view $tag --repo $Repo *> $null
  if ($LASTEXITCODE -eq 0) { throw "$tag is already released. Raise `$AppVersion in src\ProjectOptM.ps1 first (e.g. $ver -> next number)." }
  $latest = "$(& $gh release view --repo $Repo --json tagName --jq .tagName 2>$null)".Trim()
  if ($latest) {
    Say "  Latest on GitHub:  $latest"
    try { if ([version]$latest.TrimStart('v', 'V') -ge [version]$ver) { throw "The new version ($ver) must be higher than $latest." } } catch [System.Management.Automation.RuntimeException] { throw }
  }
  Say "  Publishing:        $tag" Green

  # --- Build ---
  & cmd.exe /c "`"$(Join-Path $Here 'Build.bat')`" nopause"
  if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Exe)) { throw 'The build failed - nothing was published.' }
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

  # --- Front-page files that changed ---
  $siteDir = Join-Path $Here 'repo'
  $changed = @()
  if (Test-Path $siteDir) {
    $sha1 = [System.Security.Cryptography.SHA1]::Create()
    foreach ($f in Get-ChildItem -LiteralPath $siteDir -File) {
      $bytes = [IO.File]::ReadAllBytes($f.FullName)
      $head  = [Text.Encoding]::ASCII.GetBytes("blob $($bytes.Length)`0")
      $local = -join ($sha1.ComputeHash([byte[]]($head + $bytes)) | ForEach-Object { $_.ToString('x2') })
      $remote = "$(& $gh api "repos/$Repo/contents/$($f.Name)" --jq .sha 2>$null)".Trim()
      if ($LASTEXITCODE -ne 0) { $remote = '' }
      if ($local -ne $remote) { $changed += [pscustomobject]@{ File = $f; Sha = $remote; Bytes = $bytes } }
    }
  }

  # --- Confirm ---
  Say ''
  Say '  ----------------------------------' DarkGray
  $notes -split "`n" | ForEach-Object { Say "  $_" }
  Say '  ----------------------------------' DarkGray
  Say "  Files: ProjectOptM.exe ($([math]::Round((Get-Item $Exe).Length / 1KB)) KB), ProjectOptM.bat"
  if ($changed.Count -gt 0) { Say "  Repo page updates: $(($changed | ForEach-Object { $_.File.Name }) -join ', ')" }
  Say ''
  $ok = Read-Host "  Publish $tag to github.com/$Repo ? (y/n)"
  if ($ok -notmatch '^[yY]') { Say '  Cancelled - nothing was published.' Yellow; return }

  # --- Publish ---
  & $gh release create $tag $Exe $Bat --repo $Repo --title "Project OptM $tag" --notes-file $notesFile
  if ($LASTEXITCODE -ne 0) { throw 'Publishing the release failed (see the message above).' }

  foreach ($c in $changed) {
    $body = @{ message = "Update $($c.File.Name) ($tag)"; content = [Convert]::ToBase64String($c.Bytes) }
    if ($c.Sha) { $body.sha = $c.Sha }
    $bodyFile = Join-Path $env:TEMP 'optm-upload.json'
    [IO.File]::WriteAllText($bodyFile, ($body | ConvertTo-Json -Compress))
    & $gh api -X PUT "repos/$Repo/contents/$($c.File.Name)" --input $bodyFile *> $null
    if ($LASTEXITCODE -eq 0) { Say "  Updated $($c.File.Name) on the repo page" Green }
    else { Say "  Couldn't update $($c.File.Name) on the repo page" Yellow }
    Remove-Item $bodyFile -ErrorAction SilentlyContinue
  }

  Say ''
  Say "  Done! $tag is live: https://github.com/$Repo/releases/latest" Green
  Say '  Everyone running Project OptM will be offered the update within a few hours.' Gray
}
catch {
  Say ''
  Say "  $($_.Exception.Message)" Red
}

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
#   1. read the version and channel from src\version.h (OPTM_VERSION, OPTM_CHANNEL, OPTM_PRERELEASE)
#   2. build dist\ProjectOptM.exe with Build.bat
#   3. ask what changed
#   4. publish a GitHub release with ProjectOptM.exe attached:
#        stable        tag v<version>, "(Stable)", marked latest - everyone gets it
#        experimental  tag v<version>-experimental.<N>, a pre-release - only the Experimental update channel
#        unstable      refused - unstable builds are never published
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

# Stability badges at the top of each release's notes (GitHub has no custom release labels).
# They sit between <!-- optm-badges --> markers, so they can be replaced later without touching the notes.
$Shield = 'https://img.shields.io/badge'
$BadgeStable = "![Stable]($Shield/channel-stable-2ea44f?style=for-the-badge)"
$BadgeExperimental = "![Experimental]($Shield/channel-experimental-F5A524?style=for-the-badge)"
function Badges([string]$badges, [string]$caption) { "<!-- optm-badges -->`n$badges`n`n*$caption*`n<!-- /optm-badges -->`n`n" }
function SupersededBadge([string]$newTag) { "![Superseded by $newTag]($Shield/superseded%20by-$($newTag.Replace('-', '--'))-6B7180?style=for-the-badge)" }
# Put a new badge block on an existing release (replacing its old one), keeping its notes as they are
function Set-Badges([string]$gh, [string]$tag, [string]$block) {
  $body = (& $gh release view $tag --repo $Repo --json body --jq .body) -join "`n"
  if ($LASTEXITCODE -ne 0) { return $false }
  $body = [regex]::Replace($body, '(?s)^\s*<!-- optm-badges -->.*?<!-- /optm-badges -->\s*', '')
  $file = Join-Path $env:TEMP 'optm-release-badges.md'
  [IO.File]::WriteAllText($file, $block + $body)
  & $gh release edit $tag --repo $Repo --notes-file $file *> $null
  return $LASTEXITCODE -eq 0
}

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
  # Release channel (see README > Release channels): stable = a full release, experimental = a GitHub
  # pre-release (only people who picked the Experimental update channel get it), unstable = never published
  $channel = if ($text -match '#define OPTM_CHANNEL\s+"([a-z]*)"') { $Matches[1] } else { '' }
  $preNum = if ($text -match '#define OPTM_PRERELEASE\s+([0-9]+)') { [int]$Matches[1] } else { 0 }
  if ($channel -eq 'unstable') { throw 'This is an UNSTABLE build (OPTM_CHANNEL in src\version.h) - unstable builds are never published. Set it to "experimental" or "" first.' }
  $pre = $channel -ne ''
  if ($pre) {
    if ($preNum -lt 1) { throw 'Experimental builds are numbered: set OPTM_PRERELEASE in src\version.h to 1 (or one more than the last experimental of this version).' }
    $tag = "v$ver-$channel.$preNum"
    $title = "Project OptM v$ver (Experimental $preNum)"
  } else {
    $title = "Project OptM $tag (Stable)"
  }
  if ($text -notmatch [regex]::Escape("#define OPTM_UPDATE_REPO   `"$Repo`"")) { Say "  Warning: OPTM_UPDATE_REPO in src\version.h is not '$Repo' - users won't get updates." Yellow }
  $rcParts = (@($ver.Split('.')) + @('0', '0', '0'))[0..3] -join ',\s*'
  if ($text -notmatch "OPTM_VERSION_RC\s+$rcParts\b") { Say '  Warning: OPTM_VERSION_RC in src\version.h does not match OPTM_VERSION.' Yellow }

  & $gh release view $tag --repo $Repo *> $null
  if ($LASTEXITCODE -eq 0) {
    if ($pre) { throw "$tag is already released. Raise OPTM_PRERELEASE in src\version.h first ($preNum -> $($preNum + 1))." }
    throw "$tag is already released. Raise OPTM_VERSION in src\version.h first (e.g. $ver -> next number)."
  }
  $latest = "$(& $gh release view --repo $Repo --json tagName --jq .tagName 2>$null)".Trim()   # the latest STABLE release
  if ($latest) {
    Say "  Latest stable:     $latest"
    try {
      $cmp = [version]$latest.TrimStart('v', 'V')
      if (-not $pre -and $cmp -ge [version]$ver) { throw "The new version ($ver) must be higher than $latest." }
      if ($pre -and $cmp -ge [version]$ver) { throw "An experimental $ver would be older than the stable $latest - raise OPTM_VERSION first." }
    } catch [System.Management.Automation.RuntimeException] { throw }
  }
  Say "  Publishing:        $tag  ($(if ($pre) { 'EXPERIMENTAL pre-release - only the Experimental update channel gets it' } else { 'STABLE - everyone gets it' }))" Green

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
  $notes = if ($pre) { Badges $BadgeExperimental 'Opt-in early build - only offered on Settings > Update channel: Experimental.' }
           else      { Badges $BadgeStable 'The current stable release - tested and meant for everyone.' }
  $notes += "## $title`n`n"
  if ($pre) { $notes += "> **Experimental build.** New features before the next stable release - each has been tried, but not everything is proven on every PC yet. You get it only with Settings > Update channel: Experimental.`n`n" }
  $notes += "### What's new`n" + ($items -join "`n")
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
  if ($pre) { & $gh release create $tag $Exe --repo $Repo --title $title --notes-file $notesFile --prerelease }
  else      { & $gh release create $tag $Exe --repo $Repo --title $title --notes-file $notesFile --latest }
  if ($LASTEXITCODE -ne 0) { throw 'Publishing the release failed (see the message above).' }

  # the previous release on the same channel now says it's superseded
  $prev = ''
  if (-not $pre) { $prev = $latest }
  else {
    $list = (& $gh release list --repo $Repo --limit 30 --json tagName,isPrerelease,isDraft 2>$null) -join "`n" | ConvertFrom-Json
    $prev = ($list | Where-Object { $_.isPrerelease -and -not $_.isDraft -and $_.tagName -ne $tag -and $_.tagName -like '*-experimental*' } | Select-Object -First 1).tagName
  }
  if ($prev) {
    $block = if ($pre) { Badges "$BadgeExperimental $(SupersededBadge $tag)" "An older experimental build - $tag is newer." }
             else      { Badges "$BadgeStable $(SupersededBadge $tag)" "A stable release - $tag is newer, and the app updates itself to it." }
    if (Set-Badges $gh $prev $block) { Say "  Marked $prev as superseded by $tag" Gray } else { Say "  Couldn't update the badges on $prev (the release is out anyway)" Yellow }
  }

  Say ''
  Say "  Done! $tag is live: https://github.com/$Repo/releases/tag/$tag" Green
  if ($pre) { Say '  People on the Experimental update channel will be offered it within a few hours. Stable users never see it.' Gray }
  else      { Say '  Everyone running Project OptM will be offered the update within a few hours.' Gray }
}
catch {
  Say ''
  Say "  $($_.Exception.Message)" Red
}

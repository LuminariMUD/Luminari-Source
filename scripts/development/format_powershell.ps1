#!/usr/bin/env pwsh
# Format PowerShell files in place with the pinned PSScriptAnalyzer formatter and
# the repository settings in PSScriptAnalyzerSettings.psd1. The powershell-format
# pre-commit hook runs this with the staged file names; it needs pwsh on PATH.
param(
  [Parameter(ValueFromRemainingArguments = $true)]
  [string[]]$Path
)

$ErrorActionPreference = 'Stop'
$version = '1.25.0'

if ($env:LUMINARI_FORMATTER_CACHE) {
  $cache = $env:LUMINARI_FORMATTER_CACHE
} elseif ($env:XDG_CACHE_HOME) {
  $cache = Join-Path $env:XDG_CACHE_HOME 'luminari-formatters'
} else {
  $cache = Join-Path $HOME '.cache/luminari-formatters'
}
$modules = Join-Path $cache 'psmodules'
$manifest = Join-Path $modules "PSScriptAnalyzer/$version/PSScriptAnalyzer.psd1"
if (-not (Test-Path -LiteralPath $manifest)) {
  New-Item -ItemType Directory -Force -Path $modules | Out-Null
  Save-PSResource -Name PSScriptAnalyzer -Version $version -Path $modules -TrustRepository -Quiet
}
Import-Module $manifest

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$settings = Join-Path $repoRoot 'PSScriptAnalyzerSettings.psd1'
$utf8 = [System.Text.UTF8Encoding]::new($false)
foreach ($file in $Path) {
  $fullPath = (Resolve-Path -LiteralPath $file).Path
  $original = [System.IO.File]::ReadAllText($fullPath)
  $formatted = Invoke-Formatter -ScriptDefinition $original -Settings $settings
  if ($formatted -cne $original) {
    [System.IO.File]::WriteAllText($fullPath, $formatted, $utf8)
  }
}

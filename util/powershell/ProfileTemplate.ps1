# Luminari-Source PowerShell profile snippet
# BEGIN Luminari-Source
# This block is managed by util/powershell/*.ps1
# Add your customizations below. Keep within the BEGIN/END markers so the
# installer can safely update/remove this section.

# Example: ensure PSReadLine is available and configured
try {
  Import-Module PSReadLine -ErrorAction Stop
  Set-PSReadLineOption -EditMode Windows -BellStyle None -PredictionSource History
} catch {}

# Example aliases/functions for the project
Set-Alias ll Get-ChildItem -Scope Global
function gs { git status }
function gd { git diff --no-index $args }

# Example: add repo root bin to PATH if present
$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$binPath = Join-Path $repoRoot 'bin'
if (Test-Path $binPath) {
  if (-not ($env:Path -split ';' | Where-Object { $_ -eq $binPath })) {
    $env:Path = "$binPath;$env:Path"
  }
}

# END Luminari-Source


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

# LuminariMUD world editing helpers
function luminari-aider {
  param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Paths)
  Push-Location $repoRoot
  try {
    $venv = Join-Path $repoRoot 'aider-env/Scripts/Activate.ps1'
    if (Test-Path -LiteralPath $venv) { . $venv }
    $readArgs = @('--no-git','--read','.aider.luminari.context.md','--read','.aider.luminari.prompts.md')
    if ($Paths) { aider @readArgs @Paths } else { aider @readArgs }
  } finally { Pop-Location }
}

function luminari-zone {
  param([Parameter(Mandatory = $true)][string]$Zone)
  $files = @(
    "lib/world/wld/$Zone.wld",
    "lib/world/mob/$Zone.mob",
    "lib/world/obj/$Zone.obj",
    "lib/world/zon/$Zone.zon"
  )
  luminari-aider @files
}

function backup-zone {
  param([Parameter(Mandatory = $true)][string]$Zone)
  $toolPath = Join-Path $repoRoot 'lib/world/tools/backup-zone.ps1'
  if (-not (Test-Path -LiteralPath $toolPath)) { $toolPath = Join-Path $repoRoot 'backup-zone.ps1' }
  & $toolPath -Zone $Zone
}

function validate-zone {
  param([Parameter(Mandatory = $true)][string]$Zone)
  $toolPath = Join-Path $repoRoot 'lib/world/tools/validate-zone.ps1'
  if (-not (Test-Path -LiteralPath $toolPath)) { $toolPath = Join-Path $repoRoot 'validate-zone.ps1' }
  & $toolPath -Zone $Zone
}

function check-mud {
  $exe = Join-Path $repoRoot 'bin/circle.exe'
  $alt = Join-Path $repoRoot 'bin/circle'
  if (Test-Path -LiteralPath $exe) { & $exe -c -q }
  elseif (Test-Path -LiteralPath $alt) { & $alt -c -q }
  else { Write-Warning 'circle binary not found.' }
}

function check-log {
  param([int]$Tail = 100)
  $log = Join-Path $repoRoot 'syslog'
  if (Test-Path -LiteralPath $log) { Get-Content -Path $log -Tail $Tail }
  else { Write-Warning 'syslog not found.' }
}

function check-errors {
  param([int]$Tail = 20)
  $log = Join-Path $repoRoot 'syslog'
  if (Test-Path -LiteralPath $log) {
    Select-String -Path $log -Pattern 'error' -SimpleMatch -CaseSensitive:$false | Select-Object -Last $Tail
  } else { Write-Warning 'syslog not found.' }
}

function list-zones {
  Get-ChildItem -Path (Join-Path $repoRoot 'lib/world/zon/*.zon') | Sort-Object Name | ForEach-Object { $_.BaseName }
}

function zone-info {
  param([Parameter(Mandatory = $true)][string]$Zone)
  $path = Join-Path $repoRoot ("lib/world/zon/$Zone.zon")
  if (Test-Path -LiteralPath $path) {
    Get-Content -Path $path | Where-Object { $_ -match '^#' } | Select-Object -First 3
  } else { Write-Warning "Zone file not found: $path" }
}

# END Luminari-Source


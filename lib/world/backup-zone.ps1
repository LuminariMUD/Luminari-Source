Param(
  [Parameter(Mandatory = $true)][string]$Zone
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$date = Get-Date -Format 'yyyyMMdd-HHmm'
$backupDir = Join-Path $repoRoot "lib/world/backups/$date"
New-Item -ItemType Directory -Path $backupDir -Force | Out-Null

$exts = 'wld','mob','obj','zon','trg','shp','qst','hlq'
foreach ($ext in $exts) {
  $src = Join-Path $repoRoot ("lib/world/{0}/{1}.{0}" -f $ext, $Zone)
  if (Test-Path -LiteralPath $src) {
    $dst = Join-Path $backupDir ("{0}.{1}.bak" -f $Zone, $ext)
    Copy-Item -LiteralPath $src -Destination $dst -Force
    Write-Host "Backed up $Zone.$ext"
  }
}

Write-Host "Zone $Zone backed up to $backupDir"



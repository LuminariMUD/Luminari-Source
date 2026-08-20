Param(
  [Parameter(Mandatory = $true)][string]$Zone
)

$ErrorActionPreference = 'Stop'
Write-Host "Validating zone $Zone..."

$repoRoot = Split-Path -Parent $PSScriptRoot

# Check file exists
$exts = 'wld','mob','obj','zon'
foreach ($ext in $exts) {
  $path = Join-Path $repoRoot ("lib/world/{0}/{1}.{0}" -f $ext, $Zone)
  if (-not (Test-Path -LiteralPath $path)) {
    Write-Warning "Missing $Zone.$ext"
  }
}

# Check for syntax
$mudExe = Join-Path $repoRoot 'bin/luminari.exe'
$mudSh  = Join-Path $repoRoot 'bin/luminari'
if (Test-Path -LiteralPath $mudExe) {
  & $mudExe -c -q 2>&1 | Select-String -Pattern 'error|warning' -AllMatches -CaseSensitive:$false | Where-Object { $_.Line -match $Zone }
} elseif (Test-Path -LiteralPath $mudSh) {
  & $mudSh -c -q 2>&1 | Select-String -Pattern 'error|warning' -AllMatches -CaseSensitive:$false | Where-Object { $_.Line -match $Zone }
} else {
  Write-Warning 'luminari binary not found in ./bin, skipping syntax check'
}

# Check for terminators (lines ending with ~)
Get-ChildItem -Path (Join-Path $repoRoot "lib/world/*/$Zone.*") -ErrorAction SilentlyContinue | ForEach-Object {
  $file = $_.FullName
  if (-not (Select-String -Path $file -Pattern "`~$" -Quiet)) {
    Write-Host "Potential missing terminators in: $file"
  }
}

Write-Host 'Validation complete'

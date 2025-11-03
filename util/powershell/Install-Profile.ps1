<#
Luminari-Source PowerShell profile installer

This script installs or updates a managed profile block in all or selected
PowerShell hosts for the current user (no admin required):
- Windows PowerShell 5.1 (Documents\WindowsPowerShell\Microsoft.PowerShell_profile.ps1)
- PowerShell 7+ (Documents\PowerShell\Microsoft.PowerShell_profile.ps1)
- CurrentUserAllHosts variants for both

Usage examples:
  .\Install-Profile.ps1                 # install to common hosts
  .\Install-Profile.ps1 -AllHosts       # include CurrentUserAllHosts
  .\Install-Profile.ps1 -Pwsh7Only      # only PowerShell 7
  .\Install-Profile.ps1 -WinPSOnly      # only Windows PowerShell 5.1

The script will create profile files if missing and insert/update the
managed BEGIN/END block using util/powershell/ProfileTemplate.ps1.

To uninstall, run Remove-Profile.ps1.
#>
[CmdletBinding(SupportsShouldProcess)]
param(
  [switch] $AllHosts,
  [switch] $Pwsh7Only,
  [switch] $WinPSOnly
)

$ErrorActionPreference = 'Stop'

function Get-Template {
  $templatePath = Join-Path $PSScriptRoot 'ProfileTemplate.ps1'
  if (!(Test-Path -LiteralPath $templatePath)) {
    throw "Template not found: $templatePath"
  }
  return [System.IO.File]::ReadAllText($templatePath)
}

function Ensure-ProfileFile {
  param([string]$Path)
  $dir = Split-Path -Parent $Path
  if (!(Test-Path -LiteralPath $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
  if (!(Test-Path -LiteralPath $Path)) { New-Item -ItemType File -Path $Path -Force | Out-Null }
}

function Upsert-ManagedBlock {
  param(
    [string]$Path,
    [string]$Block
  )
  $beginTag = '# BEGIN Luminari-Source'
  $endTag   = '# END Luminari-Source'
  $content = if (Test-Path -LiteralPath $Path) { [IO.File]::ReadAllText($Path) } else { '' }
  if ($content -match [regex]::Escape($beginTag) -and $content -match [regex]::Escape($endTag)) {
    $pattern = "(?s)" + [regex]::Escape($beginTag) + ".*?" + [regex]::Escape($endTag)
    $new = [regex]::Replace($content, $pattern, ($beginTag + "`r`n" + $Block.Trim("`r`n") + "`r`n" + $endTag))
  } else {
    if (-not [string]::IsNullOrWhiteSpace($content)) { $content += "`r`n`r`n" }
    $new = $content + $beginTag + "`r`n" + $Block.Trim("`r`n") + "`r`n" + $endTag + "`r`n"
  }
  [IO.File]::WriteAllText($Path, $new, (New-Object System.Text.UTF8Encoding($false)))
}

function Get-Targets {
  [OutputType([hashtable[]])]
  param()
  $currentUser = $true
  $docs = [Environment]::GetFolderPath('MyDocuments')
  $targets = @()

  if (-not $WinPSOnly) {
    $targets += @{ Name='PowerShell 7 (CurrentUser, CurrentHost)'; Path = Join-Path $docs 'PowerShell/Microsoft.PowerShell_profile.ps1' }
    if ($AllHosts) { $targets += @{ Name='PowerShell 7 (CurrentUserAllHosts)'; Path = Join-Path $docs 'PowerShell/profile.ps1' } }
  }
  if (-not $Pwsh7Only) {
    $targets += @{ Name='Windows PowerShell 5.1 (CurrentUser, CurrentHost)'; Path = Join-Path $docs 'WindowsPowerShell/Microsoft.PowerShell_profile.ps1' }
    if ($AllHosts) { $targets += @{ Name='Windows PowerShell 5.1 (CurrentUserAllHosts)'; Path = Join-Path $docs 'WindowsPowerShell/profile.ps1' } }
  }
  return $targets
}

$tpl = Get-Template
$block = ($tpl -split "\r?\n")
# Extract the content between the markers if the template includes them
$beginIdx = ($block | Select-String -SimpleMatch '# BEGIN Luminari-Source').LineNumber - 1
$endIdx   = ($block | Select-String -SimpleMatch '# END Luminari-Source').LineNumber - 1
if ($beginIdx -ge 0 -and $endIdx -gt $beginIdx) {
  $managed = ($block[($beginIdx+1)..($endIdx-1)] -join "`r`n")
} else {
  $managed = $tpl
}

$targets = Get-Targets
foreach ($t in $targets) {
  $path = $t.Path
  if ($PSCmdlet.ShouldProcess($path, 'Install/Update Luminari-Source profile block')) {
    Ensure-ProfileFile -Path $path
    Upsert-ManagedBlock -Path $path -Block $managed
    Write-Host "Installed/updated: $($t.Name) -> $path"
  }
}

Write-Host 'Done.'


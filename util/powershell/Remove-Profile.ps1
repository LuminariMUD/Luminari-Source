<#
Luminari-Source PowerShell profile uninstaller
Removes the managed BEGIN/END block from selected profile files.
#>
[CmdletBinding(SupportsShouldProcess)]
param(
  [switch] $AllHosts,
  [switch] $Pwsh7Only,
  [switch] $WinPSOnly
)

$ErrorActionPreference = 'Stop'

function Remove-ManagedBlock {
  param([string]$Path)
  if (!(Test-Path -LiteralPath $Path)) { return }
  $beginTag = '# BEGIN Luminari-Source'
  $endTag   = '# END Luminari-Source'
  $content = [IO.File]::ReadAllText($Path)
  if ($content -notmatch [regex]::Escape($beginTag)) { return }
  $pattern = "(?s)" + [regex]::Escape($beginTag) + ".*?" + [regex]::Escape($endTag)
  $new = [regex]::Replace($content, $pattern, '').TrimEnd() + "`r`n"
  [IO.File]::WriteAllText($Path, $new, (New-Object System.Text.UTF8Encoding($false)))
}

function Get-Targets {
  [OutputType([hashtable[]])]
  param()
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

$targets = Get-Targets
foreach ($t in $targets) {
  $path = $t.Path
  if ($PSCmdlet.ShouldProcess($path, 'Remove Luminari-Source profile block')) {
    Remove-ManagedBlock -Path $path
    Write-Host "Cleaned: $($t.Name) -> $path"
  }
}

Write-Host 'Done.'


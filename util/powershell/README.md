# Luminari-Source PowerShell utilities

These scripts help contributors install, update, or remove a managed profile block that enables helpful defaults for this project across PowerShell hosts.

Included
- Install-Profile.ps1: Installs/updates the profile block for the current user.
- Remove-Profile.ps1: Removes the profile block.
- ProfileTemplate.ps1: The template content inserted inside managed markers.

Supported hosts
- PowerShell 7+ (Documents/PowerShell/Microsoft.PowerShell_profile.ps1)
- Windows PowerShell 5.1 (Documents/WindowsPowerShell/Microsoft.PowerShell_profile.ps1)
- Optionally, CurrentUserAllHosts variants for both.

Usage
1) Open PowerShell 7 or Windows PowerShell 5.1.
2) From the repo root, run:
   - pwsh:
     - ./util/powershell/Install-Profile.ps1
   - Windows PowerShell 5.1:
     - powershell -ExecutionPolicy Bypass -File .\util\powershell\Install-Profile.ps1

Common options
- ./util/powershell/Install-Profile.ps1 -AllHosts
  Also installs to CurrentUserAllHosts.
- ./util/powershell/Install-Profile.ps1 -Pwsh7Only
  Only touches PowerShell 7 profile files.
- ./util/powershell/Install-Profile.ps1 -WinPSOnly
  Only touches Windows PowerShell 5.1 profile files.

Uninstall
- ./util/powershell/Remove-Profile.ps1 -AllHosts

Notes for WSL users
- You can run these from WSL using Windows PowerShell by invoking wslpath-aware commands or by running PowerShell on Windows:
  - /mnt/c/Program Files/PowerShell/7/pwsh.exe -ExecutionPolicy Bypass -File "$(wslpath -w ./util/powershell/Install-Profile.ps1)"
- The scripts target Windows profile locations under C:\Users\<User>\Documents.

Reloading profile
- After installation, reload your profile in each host:
  - . $PROFILE
  - Or restart the terminal.

Contributing
- Keep any changes to ProfileTemplate.ps1 within the BEGIN/END markers so that the installer can safely update them across hosts.

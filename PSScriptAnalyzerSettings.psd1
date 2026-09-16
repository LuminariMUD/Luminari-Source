# Formatting settings for scripts/development/format_powershell.ps1: the
# PSScriptAnalyzer 1.25.0 CodeFormattingOTBS preset with 2-space indentation,
# which matches the existing scripts.
@{
  IncludeRules = @(
    'PSPlaceOpenBrace',
    'PSPlaceCloseBrace',
    'PSUseConsistentWhitespace',
    'PSUseConsistentIndentation',
    'PSAlignAssignmentStatement',
    'PSUseCorrectCasing'
  )
  Rules        = @{
    PSPlaceOpenBrace           = @{
      Enable             = $true
      OnSameLine         = $true
      NewLineAfter       = $true
      IgnoreOneLineBlock = $true
    }
    PSPlaceCloseBrace          = @{
      Enable             = $true
      NewLineAfter       = $false
      IgnoreOneLineBlock = $true
      NoEmptyLineBefore  = $false
    }
    PSUseConsistentIndentation = @{
      Enable              = $true
      Kind                = 'space'
      PipelineIndentation = 'IncreaseIndentationForFirstPipeline'
      IndentationSize     = 2
    }
    PSUseConsistentWhitespace  = @{
      Enable                                  = $true
      CheckInnerBrace                         = $true
      CheckOpenBrace                          = $true
      CheckOpenParen                          = $true
      CheckOperator                           = $true
      CheckPipe                               = $true
      CheckPipeForRedundantWhitespace         = $false
      CheckSeparator                          = $true
      CheckParameter                          = $false
      IgnoreAssignmentOperatorInsideHashTable = $true
    }
    PSAlignAssignmentStatement = @{
      Enable         = $true
      CheckHashtable = $true
    }
    PSUseCorrectCasing         = @{
      Enable = $true
    }
  }
}

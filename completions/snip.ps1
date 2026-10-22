# PowerShell completion script for SnipCLI (snip)

Register-ArgumentCompleter -Native -CommandName snip -ScriptBlock {
    param($wordToComplete, $commandAst, $cursorPosition)

    $elements = $commandAst.Elements
    $count = $elements.Count

    $subcommands = @("add", "list", "get", "delete", "search", "stats", "help")
    $globalOpts = @("--help", "-h", "--version", "-v")
    $formats = @("terminal", "markdown")
    $languages = @("cpp", "c", "python", "rust", "go", "java", "javascript", "typescript", "bash")

    # Find if a subcommand has been typed
    $cmd = $null
    for ($i = 1; $i -lt $count; $i++) {
        $val = $elements[$i].Value
        if ($subcommands -contains $val) {
            $cmd = $val
            break
        }
    }

    if (-not $cmd) {
        if ($wordToComplete -like "-*") {
            return $globalOpts | Where-Object { $_ -like "$wordToComplete*" } | ForEach-Object {
                [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterName', $_)
            }
        } else {
            return $subcommands | Where-Object { $_ -like "$wordToComplete*" } | ForEach-Object {
                [System.Management.Automation.CompletionResult]::new($_, $_, 'Command', $_)
            }
        }
    }

    # Previous token
    $prevIndex = $count - 2
    $prev = if ($prevIndex -ge 0) { $elements[$prevIndex].Value } else { "" }

    if ($prev -eq "--format") {
        return $formats | Where-Object { $_ -like "$wordToComplete*" } | ForEach-Object {
            [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)
        }
    }

    if ($prev -in @("--lang", "--language")) {
        return $languages | Where-Object { $_ -like "$wordToComplete*" } | ForEach-Object {
            [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)
        }
    }

    $options = switch ($cmd) {
        "add"    { @("--title", "-t", "--lang", "--language", "-l", "--code", "-c", "--file", "-f", "--tags", "--tag", "--format") }
        "list"   { @("--tag", "--tags", "--lang", "--language", "-l", "--format") }
        "get"    { @("--id", "--title", "-t", "--code-only", "-c", "--format") }
        "delete" { @("--id", "--title", "-t", "--format") }
        "search" { @("--title", "-t", "--tag", "--tags", "--lang", "--language", "-l", "--format") }
        "stats"  { @("--format") }
        default  { @() }
    }

    $all = $options + $globalOpts
    return $all | Where-Object { $_ -like "$wordToComplete*" } | ForEach-Object {
        [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterName', $_)
    }
}

#!/usr/bin/env bash
# Bash completion script for SnipCLI (snip)

_snip_completions() {
    local cur prev words cword
    _init_completion || return

    local commands="add list get delete search stats help"
    local global_opts="--help -h --version -v"

    # Find the active subcommand if present
    local cmd=""
    local i
    for ((i=1; i < cword; i++)); do
        local word="${words[i]}"
        case "$word" in
            add|list|get|delete|search|stats|help)
                cmd="$word"
                break
                ;;
        esac
    done

    # If completing the subcommand or global flags
    if [[ -z "$cmd" ]]; then
        if [[ "$cur" == -* ]]; then
            COMPREPLY=($(compgen -W "$global_opts" -- "$cur"))
        else
            COMPREPLY=($(compgen -W "$commands" -- "$cur"))
        fi
        return 0
    fi

    # Subcommand specific options
    case "$cmd" in
        add)
            local add_opts="--title --lang --language --code --tags --desc --description --format"
            case "$prev" in
                --format)
                    COMPREPLY=($(compgen -W "terminal markdown" -- "$cur"))
                    return 0
                    ;;
                --lang|--language)
                    COMPREPLY=($(compgen -W "cpp c python rust go java javascript typescript bash" -- "$cur"))
                    return 0
                    ;;
                *)
                    COMPREPLY=($(compgen -W "$add_opts $global_opts" -- "$cur"))
                    return 0
                    ;;
            esac
            ;;
        list)
            local list_opts="--format"
            case "$prev" in
                --format)
                    COMPREPLY=($(compgen -W "terminal markdown" -- "$cur"))
                    return 0
                    ;;
                *)
                    COMPREPLY=($(compgen -W "$list_opts $global_opts" -- "$cur"))
                    return 0
                    ;;
            esac
            ;;
        get)
            local get_opts="--id --title --code-only --format"
            case "$prev" in
                --format)
                    COMPREPLY=($(compgen -W "terminal markdown" -- "$cur"))
                    return 0
                    ;;
                *)
                    COMPREPLY=($(compgen -W "$get_opts $global_opts" -- "$cur"))
                    return 0
                    ;;
            esac
            ;;
        delete)
            local del_opts="--id --title --format"
            case "$prev" in
                --format)
                    COMPREPLY=($(compgen -W "terminal markdown" -- "$cur"))
                    return 0
                    ;;
                *)
                    COMPREPLY=($(compgen -W "$del_opts $global_opts" -- "$cur"))
                    return 0
                    ;;
            esac
            ;;
        search)
            local search_opts="--tag --query --language --format"
            case "$prev" in
                --format)
                    COMPREPLY=($(compgen -W "terminal markdown" -- "$cur"))
                    return 0
                    ;;
                --language)
                    COMPREPLY=($(compgen -W "cpp c python rust go java javascript typescript bash" -- "$cur"))
                    return 0
                    ;;
                *)
                    COMPREPLY=($(compgen -W "$search_opts $global_opts" -- "$cur"))
                    return 0
                    ;;
            esac
            ;;
        stats)
            local stats_opts="--format"
            case "$prev" in
                --format)
                    COMPREPLY=($(compgen -W "terminal markdown" -- "$cur"))
                    return 0
                    ;;
                *)
                    COMPREPLY=($(compgen -W "$stats_opts $global_opts" -- "$cur"))
                    return 0
                    ;;
            esac
            ;;
        help)
            COMPREPLY=($(compgen -W "add list get delete search stats" -- "$cur"))
            return 0
            ;;
    esac
}

complete -F _snip_completions snip

# Fish shell completion for SnipCLI (snip)

# Disable file completions by default
complete -c snip -f

# Global options
complete -c snip -s h -l help -d "Show help information"
complete -c snip -s v -l version -d "Show version information"

# Subcommands
complete -c snip -n "__fish_use_subcommand" -a add -d "Add a new snippet"
complete -c snip -n "__fish_use_subcommand" -a list -d "List all stored snippets"
complete -c snip -n "__fish_use_subcommand" -a get -d "Retrieve a snippet by ID or Title"
complete -c snip -n "__fish_use_subcommand" -a delete -d "Delete a snippet by ID or Title"
complete -c snip -n "__fish_use_subcommand" -a search -d "Search snippets by query, tag, or language"
complete -c snip -n "__fish_use_subcommand" -a stats -d "Display repository snippet statistics"
complete -c snip -n "__fish_use_subcommand" -a help -d "Display help information"

# Subcommand: add
complete -c snip -n "__fish_seen_subcommand_from add" -s t -l title -r -d "Snippet title"
complete -c snip -n "__fish_seen_subcommand_from add" -s l -l lang -l language -r -a "cpp c python rust go java javascript typescript bash" -d "Programming language"
complete -c snip -n "__fish_seen_subcommand_from add" -s c -l code -r -d "Snippet code"
complete -c snip -n "__fish_seen_subcommand_from add" -s f -l file -r -F -d "Source code file path"
complete -c snip -n "__fish_seen_subcommand_from add" -l tag -l tags -r -d "Comma-separated tags"
complete -c snip -n "__fish_seen_subcommand_from add" -l format -r -a "terminal markdown" -d "Output format"

# Subcommand: list
complete -c snip -n "__fish_seen_subcommand_from list" -l tag -l tags -r -d "Filter by category tag"
complete -c snip -n "__fish_seen_subcommand_from list" -s l -l lang -l language -r -a "cpp c python rust go java javascript typescript bash" -d "Filter by programming language"
complete -c snip -n "__fish_seen_subcommand_from list" -l format -r -a "terminal markdown" -d "Output format"

# Subcommand: get
complete -c snip -n "__fish_seen_subcommand_from get" -l id -r -d "Snippet ID"
complete -c snip -n "__fish_seen_subcommand_from get" -s t -l title -r -d "Snippet title"
complete -c snip -n "__fish_seen_subcommand_from get" -s c -l code-only -d "Output raw code byte-exact"
complete -c snip -n "__fish_seen_subcommand_from get" -l format -r -a "terminal markdown" -d "Output format"

# Subcommand: delete
complete -c snip -n "__fish_seen_subcommand_from delete" -l id -r -d "Snippet ID"
complete -c snip -n "__fish_seen_subcommand_from delete" -s t -l title -r -d "Snippet title"
complete -c snip -n "__fish_seen_subcommand_from delete" -l format -r -a "terminal markdown" -d "Output format"

# Subcommand: search
complete -c snip -n "__fish_seen_subcommand_from search" -s t -l title -r -d "Filter by title substring"
complete -c snip -n "__fish_seen_subcommand_from search" -l tag -l tags -r -d "Filter by tag"
complete -c snip -n "__fish_seen_subcommand_from search" -s l -l lang -l language -r -a "cpp c python rust go java javascript typescript bash" -d "Filter by language"
complete -c snip -n "__fish_seen_subcommand_from search" -l format -r -a "terminal markdown" -d "Output format"

# Subcommand: stats
complete -c snip -n "__fish_seen_subcommand_from stats" -l format -r -a "terminal markdown" -d "Output format"

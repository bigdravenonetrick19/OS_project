I used ChatGPT as the AI tool

Prompts:
- generate parse_condition for field:op:value
- generate match_condition for struct Report

What AI generated:
- sscanf parsing
- basic comparisons

What I changed:
- added !=, <, > for numeric fields
- added timestamp handling
- fixed return checks
- integrated into multi-condition AND filter

What I learned:
- parsing tokens safely in C
- mapping string ops to typed comparisons
- validating AI code and extending it

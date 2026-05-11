# AI Usage – Phases 1 and 2

## Tool used
ChatGPT (OpenAI)

---

## Phase 1

### Prompts used

1. "Generate a C function parse_condition that splits field:operator:value"
2. "Generate match_condition function for struct Report"

---

### What AI generated

- parse_condition using sscanf
- match_condition with basic comparisons for:
  - severity
  - category
  - inspector

---

### What I changed

- Added support for:
  - !=, <, >, <=, >=
- Added timestamp comparisons (conversion from string to time_t)
- Fixed incorrect comparisons for strings
- Ensured correct return values (0/1)
- Integrated logic into multi-condition AND filtering
- Added validation for parsing success

---

### What I learned

- How to safely parse structured strings in C
- Differences between numeric and string comparisons
- Importance of validating AI-generated code
- How to extend incomplete AI-generated solutions

---

## Phase 2

### Prompts used

1. "How to send SIGUSR1 to another process in C"
2. "How to use sigaction instead of signal"
3. "Example of writing PID to file and reading it"

---

### What AI generated

- Basic usage of:
  - kill()
  - sigaction()
- Example signal handlers

---

### What I changed

- Integrated signal sending into add_report
- Added error handling:
  - missing .monitor_pid
  - invalid PID
  - failed kill()
- Logged all cases explicitly:
  - monitor_notified
  - monitor_not_found
  - monitor_signal_failed
- Implemented monitor_reports program lifecycle:
  - create PID file
  - delete on exit

---

### What I learned

- Difference between signal() and sigaction()
- How processes communicate via signals
- Importance of checking all failure cases
- How to safely coordinate two separate programs

---

## Final conclusion

AI was useful for generating skeleton implementations, but required:
- debugging
- extending functionality
- adapting to project requirements

All final logic and integration were implemented and verified manually.

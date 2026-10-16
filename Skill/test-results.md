# Test Results

Environment: Ubuntu under WSL, GCC 15.2. Build flags: `-Wall -Wextra -Werror -std=c11`.

`make test` completed successfully. Every source compiled as its own executable. The script ran a behavior check for each numbered stage and the retained `Skill-3b.c` exercise.

| Program | Build | Executed check | Result |
|---|---|---|---|
| Skill 1 | PASS | Read `input-one`; printed it back | PASS |
| Skill 2 | PASS | `exit` and EOF both terminate | PASS |
| Skill 3 | PASS | Recorded and listed `echo alpha` | PASS |
| Skill 3b (extra) | PASS | Tokenized a three-word command | PASS |
| Skill 4 | PASS | Split tab-separated words; argc was 2 | PASS |
| Skill 5 | PASS | Accepted a command and rejected empty input | PASS |
| Skill 6 | PASS | Preserved quotes and processed escaped whitespace | PASS |
| Skill 7 | PASS | Parent and forked child both reported | PASS |
| Skill 8 | PASS | Executed `echo`; reported exit status 0 | PASS |
| Skill 9 | PASS | Found and ran `printf` through PATH | PASS |
| Skill 10 | PASS | Expanded `$HOME` | PASS |
| Skill 11 | PASS | Ran `pwd`, changed to `/tmp`, and printed cwd | PASS |
| Skill 12 | PASS | Accepted `export OSSP_TEST=value` | PASS |
| Skill 13 | PASS | Listed history and resolved `!!` | PASS |
| Skill 14 | PASS | Two-command pipe produced byte count 3 | PASS |
| Skill 15 | PASS | Three-command pipeline produced expected count | PASS |
| Skill 16 | PASS | Created output file through `>` | PASS |
| Skill 17 | PASS | Appended output and captured stderr with `2>` | PASS |
| Skill 18 | PASS | Launched a background process | PASS |
| Skill 19 | PASS | Added and listed a background job | PASS |
| Skill 20 | PASS | Ran foreground command and exited | PASS |
| Skill 21 | PASS | Created/waited for a foreground process group | PASS |
| Skill 22 | PASS | Added a background job and cleaned up on exit | PASS |
| Skill 23 | PASS | Ran an integrated pipeline | PASS |
| ShellForge (stage 24) | PASS | Built-ins, quoting, expansion, pipeline, redirects, background job, and `$? = 127` | PASS |

Additional ShellForge PTY test passed: terminal handoff, Ctrl-Z stop, `bg`/`fg` resume, and Ctrl-C interruption. The tests do not cover every POSIX shell behavior; documented limitations are in `README.md`. No package installation or system-wide changes were made.

---
name: quota
description: Check Claude Code session, weekly, and credits usage limits with reset times. Use when the user asks about usage, limits, how much is left, when the session resets, or asks you to monitor usage to avoid surpassing a limit.
user-invocable: false
allowed-tools:
  - Bash
---

Run the script to check quota:

```
bash .claude/skills/quota/scripts/claude-usage
```

The script launches a tmux session, starts `claude`, sends `/usage`, and captures the TUI screen (~15 seconds). Read the output to determine how close each limit is and advise accordingly.

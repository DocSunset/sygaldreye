---
name: develop
description: Pull work items from kanban/ready and implement them. Use when the user asks to develop, work on, or implement kanban items, or says "start developing".
user-invocable: true
allowed-tools:
  - "*"
---

For each work item in `kanban/ready`, assess the complexity of the work item, move it to develop, set up a git worktree for the task, and dispatch a sub-agent with an appropriate model to implement the work item in the worktree and commit its changes when finished. When a sub-agent finishes a work item, move the work item to `kanban/review`. Use your judgement to decide whether to run sub-agents one at a time sequentially, or in parallel. Never allow more than 4 sub-agents to run simultaneously to avoid exhausting the system's RAM and CPU resources.

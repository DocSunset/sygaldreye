---
name: telegram
description: Send a message or image to Travis via Telegram. Use when the user asks you to send a Telegram, notify them, or send an image/file to their phone.
user-invocable: true
allowed-tools:
  - Bash
---

Credentials are in memory (do NOT put them in the repo):
- See `/home/tw/.claude/projects/-home-tw-tw-repos-eyeballs/memory/telegram-bot.md` for the bot token and chat ID.

Send a text message:
```
curl -s "https://api.telegram.org/bot<TOKEN>/sendMessage" \
  -d chat_id=<CHAT_ID> \
  -d text="..."
```

Send an image or file:
```
curl -s "https://api.telegram.org/bot<TOKEN>/sendPhoto" \
  -F chat_id=<CHAT_ID> \
  -F photo=@/absolute/path/to/image.png \
  -F caption="optional caption"
```

Use `sendDocument` instead of `sendPhoto` for non-image files.

Read the token and chat ID from the memory file before making any API call.

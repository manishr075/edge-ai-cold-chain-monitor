# Repository security note

The production sketch currently keeps credential placeholders directly in the source so the Arduino IDE upload flow remains simple.

Before publishing to GitHub:
1. Fill credentials locally only.
2. Do not commit the populated `.ino`.
3. Prefer a local `secrets.h` excluded by `.gitignore`, or another build-time secret mechanism.
4. If a ThingSpeak Write API key is ever exposed, rotate it from the channel's API Keys tab.

The embedded public CA certificate is not a secret.

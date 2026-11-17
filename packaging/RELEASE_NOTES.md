## Download

| Your computer | File to download |
|---|---|
| Mac with Apple Silicon (M1, M2, M3, M4) | `{{APP}}-{{LABEL}}-macOS-AppleSilicon.dmg` |
| Windows PC (64-bit) | `{{APP}}-{{LABEL}}-Windows-x64.zip` |

Not sure which Mac you have? Apple menu > **About This Mac**. "Chip: Apple M…" means Apple Silicon. Intel Macs are not supported.

{{APP}} installs next to the regular PrusaSlicer. It keeps its own settings, so installing it does not change your existing PrusaSlicer.

## Install on a Mac

1. Open the `.dmg` file you downloaded.
2. Drag **{{APP}}** onto the **Applications** folder.
3. Open **Applications** and double-click **{{APP}}**.
4. macOS warns that it can't check the app for malicious software. Click **Done** (not "Move to Trash").
   This happens because the app is free and not registered with Apple. It is expected.
5. Open **System Settings > Privacy & Security**, scroll down to the message about {{APP}}, and click **Open Anyway**. Enter your Mac password if asked.
6. Double-click **{{APP}}** again and click **Open**. From now on it opens normally.

## Install on Windows

1. Right-click the `.zip` file you downloaded and choose **Extract All…**, then **Extract**.
   Don't run the app from inside the zip; it won't find its files.
2. Open the extracted **{{APP}}** folder and double-click **{{APP}}.exe**.
3. If Windows shows "Windows protected your PC", click **More info**, then **Run anyway**.
   This happens because the app is free and not signed with a paid certificate. It is expected.
4. Optional: right-click **{{APP}}.exe** > **Show more options** > **Send to > Desktop (create shortcut)**.

Keep the whole folder together: the `.exe` needs the other files next to it.

## First start

The Configuration Wizard opens. Pick your printer and click **Finish**.

## Something wrong?

Open an issue on this repository and say which file you downloaded and what happened.

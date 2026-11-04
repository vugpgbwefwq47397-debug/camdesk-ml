# Installation and recovery

This binary release is CamDesk **0.4.2-hui** for **EOS 50D / Canon firmware 1.0.9 / 50D.109**. Other Magic Lantern cameras require a separate port and build.

1. Confirm Canon firmware 1.0.9 and an already working, bootable Magic Lantern CF card. First-time installations and new cards must follow the upstream 50D installation procedure first; this ZIP is an update, not an installer.
2. Connect the card to a reader and back up all contents, particularly `autoexec.bin`, `ML` and photos. Verify the release SHA-256 checksums.
3. Extract the card ZIP's `autoexec.bin`, `ML` and instruction files directly to the card root, merging directories and replacing packaged files. Do not copy an enclosing folder. Keep the core, symbol table and module from the same build together.
4. Preserve existing `ML/SETTINGS`, module-enable files and photos. Personal settings are not shipped. Formatting and Canon's firmware-update menu are not required.
5. Safely eject, insert with the camera off, then power on. Press Trash for the status screen; turn a dial for the full menu. If Classic is selected, change Interface style to Nordic in Prefs.
6. Use INFO to select a LiveView state with ML overlays enabled. For finite interval shooting, enable `camdesk.mo` in Modules, restart as prompted, then open Prefs > CamDesk. The core status screen does not require this module to be enabled.

For recovery, power off, reconnect the card and restore the complete backed-up `autoexec.bin` and `ML` set, preserving photos. Do not restore only the core while retaining mismatched symbols. After opening the card door, wait for the activity indicator or at least five seconds before removal.

Latest camera regression is pending. Reports should include camera, Canon firmware, CamDesk version, lens, shooting mode, steps and a screen photo. Discuss other-camera ports before attempting a build; do not use these 50D binaries on them.

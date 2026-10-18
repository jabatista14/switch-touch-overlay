# Installation on Atmosphere

Test target: HOS 20.5.0, Atmosphere 1.11.1, Ultrahand Overlay 2.4.1.

## Install

1. Power off the Switch and insert its microSD card into a computer, or use a
   safe SD file-transfer method.
2. Extract the release ZIP into the **root** of the microSD card. The resulting
   file must be:

   `sd:/switch/.overlays/touch_cursor.ovl`

3. Put the card back into the Switch and boot Atmosphere.
4. Open Ultrahand using your existing Ultrahand activation combo.
5. Select **Touch Cursor** from the Overlays list. Selecting it is the enable
   action; the cursor appears immediately.

No Ultrahand configuration or global keybind file is installed or changed.

## Controls

| Input | Action |
| --- | --- |
| Left or right stick | Move cursor |
| A | Touch/hold at cursor |
| ZR | Touch/hold while dragging |
| X | Recenter cursor |
| B | Release touch, disable, and close |
| L + D-pad Down + Right Stick click | Release touch and return to Ultrahand |

The cursor is cyan while hovering and orange while a virtual finger is down.
A red crossed circle means the Switch rejected access to `hid:dbg`; press B to
close safely.

## Disable or uninstall

- Press **B** at any time to send finger-up and close the overlay.
- Your normal Tesla combo (`L + Down + Right Stick click`) also sends finger-up
  and exits. The overlay reads the buttons but never rewrites the saved combo.
- To uninstall, delete only `sd:/switch/.overlays/touch_cursor.ovl` and reboot.
- If replacing an older build, overwrite that one file. No saved settings need
  to be removed.

## Troubleshooting

- Do not run sys-botbase or another tool that owns touchscreen AutoPilot at the
  same time.
- If a game misses a very quick tap, hold A slightly longer.
- If the overlay does not appear in Ultrahand, verify the exact path above and
  fully reboot Atmosphere once.
- If a red crossed circle remains visible, close with B and reboot. Persistent
  failure indicates that `hid:dbg` is unavailable or already in use.

This is an experimental accessibility overlay. Compilation can be verified on
a PC, but final behavior must be validated on real Switch hardware.

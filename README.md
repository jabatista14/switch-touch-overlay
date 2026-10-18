# Touch Cursor Overlay

An experimental Nintendo Switch Ultrahand/Tesla overlay for operating software
that expects touchscreen input when the physical touchscreen is unavailable.

The overlay reads either analog stick, draws a cursor above the foreground game,
and injects a single virtual finger through Horizon's `hid:dbg` service. It runs
entirely on the console; it does not require sys-botbase, a network connection,
or a companion PC/phone application.

## Controls

| Input | Action |
| --- | --- |
| Left or right stick | Move the cursor |
| A | Tap/hold at the cursor |
| ZR | Hold and drag a virtual finger |
| X | Recenter the cursor |
| B | Release touch and close the overlay |

The cursor is cyan while hovering and orange while touching. A red crossed
circle means that `hid:dbg` could not be initialized.

## Install

1. Build `touch_cursor.ovl`, or download it from a release made from this source.
2. Copy it to `sd:/switch/.overlays/touch_cursor.ovl`.
3. Open Ultrahand over the game and select **Touch Cursor**.

Use a current Atmosphere, Ultrahand Overlay, nx-ovlloader, and libnx-compatible
firmware combination. Do not run sys-botbase or another program that controls
touchscreen AutoPilot at the same time.

## Build

Install devkitPro with devkitA64 and libnx, plus the libraries required by
libultrahand, then run:

```sh
dkp-pacman -S switch-curl switch-zlib switch-mbedtls
make -j6
```

The output is `touch_cursor.ovl`.

## Design notes and limitations

- The overlay uses a 96x96 transparent framebuffer that follows the pointer.
  This avoids the memory cost of a full-screen double-buffered overlay.
- Touch coordinates use the Switch's native 1280x720 touchscreen coordinate
  space and are independent of docked display resolution.
- Ultrahand normally gives the active overlay exclusive physical controller
  input. Only the synthesized touchscreen events are intended to reach the game.
- Some games sample touch at 30 Hz. If a very quick tap is missed, briefly hold A.
- This first release needs real-hardware validation across firmware versions.

## Credits and license

Built with [libultrahand](https://github.com/ppkantorski/libultrahand), based on
the project structure of
[Tetris-Overlay](https://github.com/ppkantorski/Tetris-Overlay), and informed by
the open-source touch AutoPilot implementation in
[sys-botbase](https://github.com/olliz0r/sys-botbase).

Licensed under GPLv2. See `LICENSE` and the licenses bundled with libultrahand.

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
| L + D-pad Down + Right Stick click | Release touch and return to the overlay menu |

The cursor is cyan while hovering and orange while touching. A red crossed
circle means that `hid:dbg` could not be initialized.

## Install

1. Extract the release ZIP to the root of the SD card, or copy
   `touch_cursor.ovl` to `sd:/switch/.overlays/touch_cursor.ovl`.
2. Open Ultrahand over the game and select **Touch Cursor**. This enables touch
   injection immediately.
3. Press **B** to release any active touch, disable injection, and close.

The overlay does not install or modify any Ultrahand global keybind. It uses
your existing Ultrahand activation combo. To let the underlying application
receive virtual touches, it shares controller input with that application
while Touch Cursor is open. Thus sticks and buttons may also affect the game;
close the overlay with B when finished. See [INSTALL.md](INSTALL.md) for the
full SD-card guide.

Use a current Atmosphere, Ultrahand Overlay, nx-ovlloader, and libnx-compatible
firmware combination. Do not run sys-botbase or another program that controls
touchscreen AutoPilot at the same time.

The release build targets the user's HOS 20.5.0 / Atmosphere 1.11.1 /
Ultrahand 2.4.1 setup. libultrahand is linked into the `.ovl`, so the overlay
does not depend on a matching libultrahand DLL or runtime on the SD card.

## Build

Install devkitPro with devkitA64 and libnx, plus the libraries required by
libultrahand, then run:

```sh
dkp-pacman -S switch-curl switch-zlib switch-mbedtls
make -j6
```

The output is `touch_cursor.ovl`.

## Design notes and limitations

- The overlay uses a 960x540 transparent framebuffer and only pans it when the
  cursor nears an edge. Each framebuffer slot is cleared before redrawing the
  cursor; without that clear, old positions remain as colored trails.
- Cursor position and touch state are atomic because input and rendering run on
  separate threads.
- Touch coordinates use the Switch's native 1280x720 touchscreen coordinate
  space and are independent of docked display resolution.
- Ultrahand normally gives the active overlay exclusive physical controller
  input, which also suppresses input for the foreground application. This
  overlay restores shared input on show so the application can see injected
  touches. Physical button presses may also reach the application. It opts
  out of Ultrahand's own touch handling so its virtual finger cannot swallow
  A release or trigger an unintended hide.
- Ultrahand's launch combo explicitly closes this overlay. Other hide events
  only release the virtual finger, rather than terminating the process.
- Some games sample touch at 30 Hz. If a very quick tap is missed, briefly hold A.
- This first release needs real-hardware validation across firmware versions.

## Credits and license

Built with [libultrahand](https://github.com/ppkantorski/libultrahand), based on
the project structure of
[Tetris-Overlay](https://github.com/ppkantorski/Tetris-Overlay), and informed by
the open-source touch AutoPilot implementation in
[sys-botbase](https://github.com/olliz0r/sys-botbase).

Licensed under GPLv2. See `LICENSE` and the licenses bundled with libultrahand.

# HB App Store submission draft

Use the [official submission form](https://submit.fortheusers.org/) after the
current build has been checked on hardware and uploaded as a GitHub release.
The form does not upload the executable; reviewers obtain it from the app URL.

| Field | Value |
| --- | --- |
| Console | Switch |
| App title | Touch Cursor Overlay |
| Author | Jackson / jabatista14 |
| Category | Tools (`tool`) |
| Contact | GitHub: jabatista14 (confirm preferred contact before submission) |
| Version | 0.1.4 |
| License | GPLv2 |
| App URL | https://github.com/jabatista14/switch-touch-overlay |
| Short description | Touch cursor for damaged Switch screens |
| Banner | `assets/banner.png` (848 x 208) |
| Icon | `assets/icon.png` (256 x 150) |

## Detailed description

Experimental accessibility overlay for a modded Nintendo Switch with a damaged
touchscreen. Requires Atmosphere, nx-ovlloader, and Ultrahand/Tesla. Move the
cursor with either stick; A taps/holds, ZR drags, X recenters, and B closes.
It injects a virtual finger through hid:dbg. On HOS 14+, it attempts to block
ordinary controller navigation in the foreground software while visible,
leaving touch enabled; pad input is restored when the overlay hides or exits.
It does not change your Ultrahand keybind. Built for HOS 20.5.0 / Atmosphere
1.11.1 / Ultrahand 2.4.1; compatibility with other setups is unverified. Do
not run another touchscreen AutoPilot controller at the same time. Extract the
release ZIP to the SD root.

## Changelog

Initial public package: joystick cursor, virtual taps/dragging, clean exit,
and temporary pad-only foreground input filter. Visual warning indicates if
touch injection or pad filtering fails.

## Before submission

- [ ] Confirm v0.1.4 still injects real touch on hardware.
- [ ] Confirm normal controller navigation stops while the cursor is visible.
- [ ] Confirm pad input returns after B or the Tesla activation combo.
- [ ] Upload the matching ZIP and `.ovl` as a GitHub release.
- [ ] Confirm a usable contact channel for store maintainers.

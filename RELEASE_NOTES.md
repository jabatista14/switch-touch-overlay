Touch Cursor is an experimental accessibility overlay for modded Nintendo Switch
systems with an unusable touchscreen. Move the cursor with either stick and use
A for a tap or hold, or ZR to drag. B closes it. It uses the existing Ultrahand
launch combo rather than replacing a global keybind.

Install by extracting the SD ZIP to the root of the microSD card. The overlay
file should end up at `sd:/switch/.overlays/touch_cursor.ovl`. See `INSTALL.md`
inside the ZIP for controls, removal, and troubleshooting.

This prerelease uses `hid:dbg` touch AutoPilot and attempts a HOS 14+ pad-input
filter while the cursor is visible. Virtual touch has been observed on hardware;
the new pad-filter behavior still needs user confirmation. It was built for
HOS 20.5.0 / Atmosphere 1.11.1 / Ultrahand 2.4.1. Compatibility across other
versions and applications is not established. Do not run another touchscreen
AutoPilot controller at the same time.

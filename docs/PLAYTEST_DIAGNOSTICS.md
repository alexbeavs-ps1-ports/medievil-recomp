# Playtest diagnostics

Input Recording and Input Display are enabled by default in the launcher's
Diagnostics group. The display shows the accepted controller buttons, frame
counter and REC/PLAY status. Hiding Input Display keeps recording enabled;
turn off Input Recording to stop recording. Diagnostics pause during netplay;
verified replay supports offline sessions.

When a problem occurs, press L3 + R3 to mark it, then exit normally and zip the
newest session directory named in `diagnostics-location.txt` beside the game.
Sessions normally live under `saves/diagnostics`; a writable fallback is used
when the installation directory is read-only. Include a description of the
problem and when it occurred. The bundle contains accepted inputs (including
each controller's connection state), rotating RAM checks, settings, starting
memory cards and timing/CD/render evidence. It excludes disc and BIOS images.

Developers can replay the session with `replay-diagnostics.py`, staged next to
the executable, using the same executable and legally supplied disc/BIOS. Run
`python replay-diagnostics.py --help` for arguments. The tool creates an isolated
replay directory, restores the recorded settings and memory cards, and stops
at the first input or RAM mismatch. Identity or baseline failures are errors;
an input-only run does not qualify as a verified replay. Automated replay is
muted; normal interactive launches retain their configured audio.

HD texture packs are excluded from this title's bundled mod catalog.

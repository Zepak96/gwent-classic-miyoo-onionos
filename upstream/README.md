# Pinned upstream source

This directory contains the unmodified game source files from
`asundr/gwent-classic` at commit
`08592d847f2573440a34deb9f32bc855b2544cdd`.

The matching upstream images, SVGs, and sound effects are copied under
`assets/gwent-classic/`. Keep the files in this directory unchanged; the
native Miyoo port must be checked against them whenever game behavior or UI
is implemented.

The reference is a browser game (`index.html`, `style.css`, and JavaScript).
The Miyoo release is a native SDL port, so browser interactions and rendering
must be translated while preserving their source behavior and layout.

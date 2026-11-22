## hyprlauncher
A multipurpose and versatile launcher / picker for Hyprland

![](./assets/preview.png)

## Features

- Various providers: Desktop, Unicode, Emoji, Math, Font ...
- Speedy: Fast, multi-threaded fuzzy searching
- Daemon by default: instant opening of the launcher
- Entry frequency caching: commonly used entries appear above others
- Manual entry providing: make a simple selector from your own list

## Runtime dependencies

- Desktop: none
- Unicode: `wl-copy`
- Math: `wl-copy` for copying the result
- Font: `wl-copy` for copying the result

## Desktop search

Queries are case-insensitive and Unicode-normalized. Exact application names
come first, followed by name prefixes, name substrings, and matching generic
names or keywords. Each group is sorted alphabetically. Name/keyword aliases
come from the installed desktop entries; no default application is inferred
from the compositor configuration.

If there are no literal matches, queries of 4–7 characters allow one edit;
queries of 8 or more allow two. Adjacent transpositions count as one edit.
Short queries have no typo fallback. Empty queries retain the frequently used
application list. Other finders keep their existing search behavior.

GLib is required to build and run the desktop search implementation.

Run regression checks with `ctest --test-dir build --output-on-failure`.

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


## Icon lookup

Application icons use Hyprtoolkit's native icon lookup and image renderer. Icon
size follows the result row height. Missing icons use standard theme icons
selected from desktop-entry categories and terminal metadata, with a built-in
terminal outline as a final fallback. Setting `finders:desktop_icons` to false
hides both application icons and fallbacks. No additional dependencies or
launcher-specific disk cache are required.

Size-aware asset selection, absolute icon paths, and detection of invalid artwork
require the companion Hyprtoolkit icon-resolution fix. With that fix, the toolkit
selects artwork for the row size and display scale and renders SVGs at that size.

To customize a particular application's icon, use a user desktop-entry override
in `$XDG_DATA_HOME/applications` (normally `~/.local/share/applications`), preserving
the original desktop file name and changing its `Icon` entry. The launcher does
not ship per-application artwork or override installed app branding.

With `BUILD_TESTING` enabled (the default), run the icon regression test after
building with:

```sh
ctest --test-dir build --output-on-failure -R application-icons
```

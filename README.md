## hyprlauncher
A multipurpose and versatile launcher / picker for Hyprland

![](./assets/preview.png)

## Features

- Various providers: Desktop, Unicode, Emoji, Math, Font ...
- Speedy: Fast, multi-threaded fuzzy searching
- Daemon by default: instant opening of the launcher
- Entry frequency caching: commonly used entries appear above others
- Manual entry providing: make a simple selector from your own list

## Usage

```
hyprlauncher [arg [...]]
```

### Arguments

| Flag | Description |
|------|-------------|
| `-d`, `--daemon` | Start in background without opening the window |
| `-o`, `--options "a,b,c"` | Pass an explicit comma-separated option list |
| `-m`, `--dmenu` | Read options from stdin (newline-separated, dmenu-style) |
| `-t`, `--toggle` | Toggle the launcher window (open/close) |
| `-h`, `--help` | Print help menu |
| `-v`, `--version` | Print version info |
| `--quiet` | Disable all logging |
| `--verbose` | Enable verbose logging |

### Finder prefixes

Type a prefix character to switch to a specific finder:

| Prefix | Finder | Description |
|--------|--------|-------------|
| *(none)* | Desktop | Application launcher (default) |
| `.` | Unicode | Unicode character picker (copies with `wl-copy`) |
| `=` | Math | Math expression evaluator (copies result with `wl-copy`) |
| `'` | Font | System font picker (copies font name with `wl-copy`) |

### Examples

```bash
# Start as a daemon (background), then toggle with a keybinding
hyprlauncher -d

# Power menu example

# Use as a dmenu replacement (pipe options via stdin)
echo -e "Power Off\nReboot\nSuspend" | hyprlauncher -m

# Pass explicit options (comma-separated)
hyprlauncher -o "Power Off,Reboot,Suspend"
```

### Hyprland

```ini
# Minimal Hyprland config to use with daemon mode, hyprlang (`~/.config/hypr/hyprland.conf`):
exec-once = hyprlauncher -d
bind = $mainMod, R, exec, hyprlauncher -t
```
```lua
-- Minimal Hyprland config to use with daemon mode, lua (`~/.config/hypr/hyprland.lua`):
hl.exec_cmd("hyprlauncher -d")
hl.bind(mainMod .. " + R", hl.dsp.exec_cmd("hyprlauncher -t"))
```

## Configuration

Configuration file: `~/.config/hypr/hyprlauncher.conf`

The file uses [Hyprlang](https://github.com/hyprwm/hyprlang) syntax. All options are optional — the launcher works with no config file.

### Using a custom config path

The config path follows the XDG Base Directory specification (`$XDG_CONFIG_HOME/hypr/hyprlauncher.conf`). You can override it by setting `XDG_CONFIG_HOME` before launching:

```bash
XDG_CONFIG_HOME=/tmp hyprlauncher
```

This looks for the config at `/tmp/hypr/hyprlauncher.conf`. Useful for NixOS + Home Manager users who want to test configuration changes without rebuilding:

```bash
mkdir -p /tmp/hypr
cp ~/.config/hypr/hyprlauncher.conf /tmp/hypr/
# edit /tmp/hypr/hyprlauncher.conf as needed
XDG_CONFIG_HOME=/tmp hyprlauncher
```

### General

```ini
general {
    # Grab keyboard focus when the launcher opens
    grab_focus = true

    # Show desktop applications immediately when opening (without typing)
    show_apps_on_open = false
}
```

### Finders

```ini
finders {
    # Which finder to use when no prefix matches (desktop, unicode, math)
    default_finder = desktop

    # Prefix prepended to the Exec command when launching desktop apps.
    # Useful for process managers like uwsm, systemd-run, or hyprctl dispatch.
    # Examples:
    #   desktop_launch_prefix = uwsm app --
    #   desktop_launch_prefix = hyprctl dispatch exec --
    #   desktop_launch_prefix = systemd-run --user --scope --
    desktop_launch_prefix =

    # Terminal emulator command for apps with Terminal=true in their .desktop file.
    # Include the execute flag so the app command is appended directly.
    # Examples:
    #   desktop_terminal = kitty -e
    #   desktop_terminal = wezterm -e
    #   desktop_terminal = alacritty -e
    #   desktop_terminal = foot -e
    desktop_terminal =

    # Show application icons in desktop finder results
    desktop_icons = true

    # Prefix characters for each finder
    desktop_prefix =
    unicode_prefix = .
    math_prefix = =
    font_prefix = '
}
```

### Locale

```ini
locale {
    # Override system locale (empty = use system locale)
    override =
}
```

### Cache

```ini
cache {
    # Enable frequency caching (frequently used entries rank higher)
    enabled = true
}
```

### UI

```ini
ui {
    # Window size in pixels (width height)
    window_size = 400 260
}
```

## Runtime dependencies

- Desktop: none
- Unicode: `wl-copy`
- Math: `wl-copy` for copying the result
- Font: `wl-copy` for copying the result

## TODO

- [ ] Add `-c` / `--config` flag to override the configuration file path

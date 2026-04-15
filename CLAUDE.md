# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

mport-manager is a single-file GTK+ 3 graphical package manager for MidnightBSD. It wraps `libmport` (MidnightBSD's native package management library) with a GUI. The entire application lives in `mport-manager.c`.

The application must be run as root (`doas mport-manager`) because package operations require elevated privileges.

## Build Commands

```sh
# Build (cleans first, then compiles)
make

# Install to /usr/local
sudo make install

# Clean build artifacts
make clean
```

There are no tests. CI uses `make all` via Jenkinsfile.

## Dependencies

- GTK+ 3.x (`pkg-config --cflags/--libs gtk+-3.0`)
- `libmport` — MidnightBSD package management library (`<mport.h>`)
- `libarchive`, `libbz2`, `liblzma`, `libz`, `libfetch`, `libsqlite3`, `libmd`, `libpthread`, `libX11`

The build uses `clang` with `-std=c99 -Wall -pedantic`.

## Architecture

The UI is organized into three tabs via `GtkStack`/`GtkStackSwitcher`:

1. **Available** — search the remote package index; shows results in `tree` (`GtkTreeView`); selecting a row populates a detail pane (`struct available_detail`) with name, version, comment, license, and an install button.
2. **Installed Software** — lists currently installed packages in `installedTree`; has a "Remove Application" button.
3. **Updates** — lists packages with available upgrades in `updateTree` (columns: title, version, OS release, new version); has an "Upgrade Installed Software" button.

Key global state: `mportInstance *mport` (initialized in `main`, kept for the application lifetime), and `char selectedInstalled[256]` (tracks the selected row in the Installed tab).

### libmport Integration

The app registers five callbacks on the `mportInstance` struct:

| Field | Function | Purpose |
|---|---|---|
| `msg_cb` | `mport_gtk_msg_cb` | Display informational messages |
| `confirm_cb` | `mport_gtk_confirm_cb` | Yes/no dialog prompts |
| `progress_init_cb` | `mport_gtk_progress_init_cb` | Initialize the progress bar |
| `progress_step_cb` | `mport_gtk_progress_step_cb` | Advance the progress bar |
| `progress_free_cb` | `mport_gtk_progress_free_cb` | Reset/hide the progress bar |

Core mport operations used: `mport_instance_init`, `mport_index_load`, `mport_index_lookup_pkgname` (via `lookupIndex`), `mport_install_primative` (via `install`), `mport_delete_primative` (via `delete`), and `mport_check_preconditions` (via `indexCheck`).

### libmport Version Compatibility

- Requires libmport 2.2.0+ (breaking API change at that version)
- `#ifdef DISPATCH` guards wrap `libdispatch`-dependent sections for MidnightBSD 0.7 compatibility where libdispatch lacked blocks support

## Icon and Desktop Integration

The icon is installed to both `/usr/local/share/mport/icon.png` and the hicolor theme path. `ICONFILE` macro in the source points to the runtime icon location. The `.desktop` file enables application menu integration.

# mport-manager
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2FMidnightBSD%2Fmport-manager.svg?type=shield)](https://app.fossa.io/projects/git%2Bgithub.com%2FMidnightBSD%2Fmport-manager?ref=badge_shield)

MidnightBSD graphical package manager application.

A graphical package manager for MidnightBSD 0.8 and higher. 

Uses libmport included with the operating system.

To build:
Install Gtk+ 3.x  (for 0.2.3 and lower) or Gtk 4.x for 0.3.0
make

To Install:
sudo make install

To run:
Launch mport Package Manager from the application menu, or run
`pkexec /usr/local/bin/mport-manager`. The desktop entry uses `pkexec`, and
`make install` installs `org.midnightbsd.mport-manager.policy` with an action
for `/usr/local/bin/mport-manager`. Polkit and a graphical authentication agent
must be installed for menu launches. On Midnight Workspace, the installed
autostart entry starts polkit-gnome when it is available.

Polkit authentication also requires an active login session. When starting X11
with a plain `.xinitrc` and ConsoleKit installed, launch the desktop with
`exec ck-launch-session midnightbsd-session` (or wrap the chosen window manager
in the same way). A display manager that already registered the session should
launch the desktop directly instead of nesting a ConsoleKit session.

## Dependencies
GTK 4
policykit / pkexec
polkit-gnome for graphical authentication in Midnight Workspace
libmport

## Breaking Changes
0.3.0 requires libmport 2.7.4 or higher and was tested primarily with 2.7.9 on MidnightBSD 4.0.5

0.2.3 requires libmport 2.6.2 or higher

0.2.2 requires libmport 2.4.2 or higher

0.2.1 requires libmport 2.2.8 or higher

0.2.0 requires libmport 2.2.0 through 2.2.5

0.1.5 Currently, this only works with libmport 2.1.5 and lower. There is a public API change around that release. 
NOTE:
0.7 is partially supported as long as you're running a recent
stable rather than the ISO for the release.

As libdispatch was not compiled with blocks support in 0.7,
ifdef's with DISPATCH are guarding those sections.


## License
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2FMidnightBSD%2Fmport-manager.svg?type=large)](https://app.fossa.io/projects/git%2Bgithub.com%2FMidnightBSD%2Fmport-manager?ref=badge_large)

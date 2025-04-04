# mport-manager
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2FMidnightBSD%2Fmport-manager.svg?type=shield)](https://app.fossa.io/projects/git%2Bgithub.com%2FMidnightBSD%2Fmport-manager?ref=badge_shield)

MidnightBSD graphical package manager application.

A graphical package manager for MidnightBSD 0.8 and higher. 

Uses libmport included with the operating system.

To build:
Install Gtk+ 3.x
make

To Install:
sudo make install

To run:
sudo mport-manager

## Breaking Changes
0.2.3 requires libmport 2.6.2 or higher

0.2.2 requires libmport 2.4.2 or higher

0.2.1 requires libmport 2.2.8 or higher

0.2.0 requires libmport 2.2.0 through 2.2.5

0.1.5 Currently, this only works with libmport 2.1.5 and lower. There is a public API change around that release. 
NOTE:
0.7 is partially supported as long as you're running a recent
stable rather than the ISO for the release.

As libdispatch was not complied with blocks support in 0.7,
ifdef's with DISPATCH are guarding those sections.


## License
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2FMidnightBSD%2Fmport-manager.svg?type=large)](https://app.fossa.io/projects/git%2Bgithub.com%2FMidnightBSD%2Fmport-manager?ref=badge_large)

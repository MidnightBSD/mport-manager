# mport-manager
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


NOTE:
0.7 is partially supported as long as you're runing a recent
stable rather than the ISO for the release.

As libdispatch was not complied with blocks support in 0.7,
ifdef's with DISPATCH are guarding those sections.

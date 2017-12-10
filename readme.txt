DirectIO - direct access to I/O ports

I wrote this driver primarily for pciutils, which originally depended on the
WinIo library on Windows. The WinIo license is not GPL-compatible, and
DirectIO solves this.

Warning: while the DirectIO kernel driver is loaded, it allows access to I/O
ports any to user (this includes unprivileged users).

Compiling:
- Driver should be compiled with Windows Driver Kit - start the appropriate
  build environment command line, cd to the driver directory and type nmake.
- Interface library is intended to be compiled with MinGW - I cross-compile
  from Linux. Just type make in the library directory. Note that both 32 and
  64-bit compilers are expected to be in PATH.

The compiled driver shipped in the bin directory is digitally signed, and will
load on x64 versions of Vista and Windows 7.

This has been tested to work with pciutils (and to not BSOD my machine). Use
at your own risk.

TODO:
- include the drivers as resources in library, load from there
- documentation


Jernej Simončič

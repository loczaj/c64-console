# c64-console
c64-console is a hobby project connecting Commodore 64 with Linux PC. With the help of this application and a simple cable Commodore 64 can be connected with a Linux PC. The PC can be used as a console for the good old C64.

Using a special kernel module and a special console emulator program, keys hit on PC keyboard are forwarded to C64. In the same way characters written by Commodore are forwarded to PC and displayed by the console emulator.

The project consists of 3 parts. A "server" program written in C64 assembly, a Linux kernel module, and the console emulator program.

Content of the directory tree:
------------------------------

c64/:		C64-side part of the c64-console project.
c64/Makefile:	Makefile for the 6502 assembly
c64/loader.bas: Basic loader of the server program.
c64/server.asm:	Server assembly source code (commented).
c64/server.prg: Loadable server program.

docs/:		Documentation.
docs/cable:	Specifications of the used cable.
docs/copying:	GPL license.
docs/install:	Install guide.
docs/manual:	Manual.

linux-console/:			Ncurses based C64 console emulator.
linux-console/Makefile:		Makefile for the console program.
linux-console/console.c:	Source of the console program.

linux-module/:			The c64_console kernel module.
linux-module/Makefile:		Makefile for c64_console kernel module.
linux-module/c64_console.c:	Source of c64_console kernel module.
linux-module/command_seq:	Basic command sequence for testing purposes.


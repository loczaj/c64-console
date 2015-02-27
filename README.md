## c64-console

c64-console is a hobby project connecting Commodore 64 with Linux PC. With the help of this application and a simple cable Commodore 64 can be connected with a Linux PC. The PC can be used as a console for the good old C64.

Using a special kernel module and a special console emulator program, keys hit on PC keyboard are forwarded to C64. In the same way characters written by Commodore are forwarded to PC and displayed by the console emulator.

The project consists of 3 parts. A "connector" program written in C64 assembly, a Linux kernel module named "c64_console", and the console emulator program.

The project was developed as a hobby gadget in 2002-2003.

### Content of the directory tree:

|      Path      |                   Description                      |
| -------------- | -------------------------------------------------- |
| c64/           | C64-side of the project                            |
| c64/Makefile   | Makefile of C64 connector program                  |
| c64/loader.bas | Basic loader of C64 connector program (generated)  |
| c64/server.asm | C64 connector source code written in 6502 assembly |
| c64/server.prg | C64 loadable connector program image (generated)   |
|                |                                  |
| docs/          | Documentation                    |
| docs/cable     | Specifications of the cable used |
| docs/copying   | GPL license                      |
| docs/install   | Install guide                    |
| docs/manual    | Manual                           |
|                            |                                               |
| linux-console/             | C64 console emulator program based on Ncurses |
| linux-console/Makefile     | Makefile of console program                   |
| linux-console/console.c    | Source of console program                     |
|                            |                                             |
| linux-module/              | The c64_console kernel module               |
| linux-module/Makefile      | Makefile of c64_console kernel module       |
| linux-module/c64_console.c | Source of c64_console kernel module         |
| linux-module/command_seq   | Basic command sequence for testing purposes |


## c64-console

c64-console is a hobby project connecting Commodore 64 with Linux PC. With the help of this application and a simple cable Commodore 64 can be connected with a Linux PC. The PC can be used as a console for the good old C64.

Using a special kernel module and a special console emulator program, keys hit on PC keyboard are forwarded to C64. In the same way characters written by Commodore are forwarded to PC and displayed by the console emulator. This allows acces to the C64 text console via the PC's keyboard and monitor.

The project consists of 3 parts:
- Adapter program written in C64 assembly
- Linux device driver (kernel module) named "c64_console"
- Console emulator program

The project was developed as a hobby gadget in 2002-2003.

### Content of the directory tree:

|       Path        |                  Description                     |
| ----------------- | ------------------------------------------------ |
| c64/              | C64-side of the project                          |
| c64/Makefile      | Makefile of C64 adapter program                  |
| c64/adapter.asm   | C64 adapter source code written in 6502 assembly |
| c64/adapter.prg   | C64 loadable adapter program image (generated)   |
| c64/loader.bas    | Basic loader of C64 adapter program (generated)  |
|                   |                                  |
| docs/             | Documentation                    |
| docs/cable.txt    | Specifications of the cable used |
| docs/copying.txt  | GPL license                      |
| docs/install.txt  | Install guide                    |
| docs/manual.txt   | Manual                           |
|                            |                                               |
| linux-console/             | C64 console emulator program based on Ncurses |
| linux-console/Makefile     | Makefile of console program                   |
| linux-console/console.c    | Source of console program                     |
|                            |                                               |
| linux-driver/              | The c64_console Linux device driver         |
| linux-driver/Makefile      | Makefile of c64_console device driver       |
| linux-driver/c64_console.c | Source of c64_console device driver         |
| linux-driver/command_seq   | Basic command sequence for testing purposes |


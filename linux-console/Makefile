#   **********************************************
#                                                 
#    Makefile for "console",
#     
#    C64 CONSOLE EMULATION                        
#    Based on the linux kernel module c64_console 
#                                                 
#   **********************************************
#    Lohner Roland, 2002.                         
#   **********************                        

CC=	gcc 
CFLAGS= -Wall -Wstrict-prototypes -O2

# Ugly, but necessary compilation rule
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

# Targets
default: console.o
	$(CC) -o console console.o -lncurses

clean:
	rm -f *.o
	rm -f console


#  *********************************************
#
#   C64_CONSOLE
#   Linux kernel module for connecting C64 & PC
#
#   Kernel version 2.4.x
#
#  *********************************************
#   Lohner Roland, 2002.
#  **********************
#

# Kernel module name
MODULE = c64_console

# GNU C Compiler
CC = gcc

# Compilation flags
MODCFLAGS := -D__KERNEL__ -DMODULE -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -fno-strict-aliasing -fno-common -mpreferred-stack-boundary=2 -pipe

# Include file search path and compilation flags
DEFINC = /usr/src/linux/include
INCLUDES = -I$(DEFINC)

# Ugly, but necessary compilation rule
%.o: %.c
	$(CC) -c $(MODCFLAGS) $(INCLUDES) $< -o $@

# Targets
default : $(MODULE).o

clean : 
	rm -f $(MODULE).o


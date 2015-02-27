#  *********************************************
#
#   Makefile for "adapter.asm",
#   C64 assembly for connecting C64 & PC
#
#   For use with the linux kernel module 
#   "c64_console"
#
#  *********************************************
#   Lohner Roland, 2002.
#  **********************
#

default : compile

all : loader

compile : adapter.asm
	dasm adapter.asm -f1 -oadapter.prg

loader : compile
	echo "10 I=49152" > loader.bas
	echo "20 READ B: IF B=-1 THEN END" >> loader.bas
	echo "30 POKE I,B: I=I+1: GOTO 20" >> loader.bas
	
	od -tuC -w6 -j2 -Ao adapter.prg | awk -F" " '{print 10000+(strtonum($$1)-2) " DATA " $$2 "," $$3 "," $$4 "," $$5 "," $$6 "," $$7}' >> loader.bas
	
	echo "63999 DATA -1" >> loader.bas

clean :
	rm -f adapter.prg
	rm -f loader.bas


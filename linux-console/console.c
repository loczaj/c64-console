/* **********************************************
 * 
 *  console.c v1.20
 *  
 *  C64 CONSOLE EMULATION
 *  Based on the linux kernel module c64_console
 * 
 * **********************************************
 *  Lohner Roland, 2003.
 * **********************
 */

//////////////////////
//  Standard includes
//////////////////////
//
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/poll.h>

//////////////////////////////////////////////
//  Ncurses for extended console capabilities
//////////////////////////////////////////////
//
#include <ncurses.h>

/////////////////////
//  Global variables
/////////////////////
//
WINDOW * screen;
WINDOW * win;
int device_in;
int device_out;
bool quotation = 0; 


///////////////////////////////////
//  Initialise the ncurses console
///////////////////////////////////
//
void
InitCurses(void)
{
	// Init window
	//
	screen = initscr();
	
	if ((LINES < 25) || (COLS < 40))
	{
		refresh();
		endwin();
		
		printf("Minimal screen size: 40x25\n");
		exit(1);
	}
	
	win = subwin(screen, 25, 40, (LINES - 25) / 2, (COLS - 40) / 2);
	touchwin(screen);
	
	// Set properties
	//
	noecho();		/* need no echo */
	scrollok(win, TRUE);	/* need scroll */
	keypad(win, TRUE);	/* enable keypad */
	meta(win, TRUE);	/* return 8 bits */
	cbreak();		/* get stdin immediately */
	curs_set(1);		/* block cursor */
	
	// Set color mode
	//
	start_color();  
	init_pair(1, COLOR_CYAN, COLOR_BLUE);
	wcolor_set(win, 1, NULL);
	wbkgd(win, COLOR_PAIR(1));	
}


////////////////////////////////////////
//  Convert character to C64 ascii code
////////////////////////////////////////
//
void
convert(int* byte)
{
	switch (*byte)
	{
		case KEY_DC:		*byte = 20;  break;     
		case KEY_UP:		*byte = 145; break;
		case KEY_DOWN:		*byte = 17;  break;
		case KEY_LEFT:		*byte = 157; break;
		case KEY_RIGHT:		*byte = 29;  break;     
		case KEY_BACKSPACE:	*byte = 20;  break;
		case KEY_IC:		*byte = 148; break;
		case KEY_HOME:		*byte = 19;  break;
		case KEY_END:		*byte = 147; break;
					
		case KEY_F(1): *byte = 133;  break;     
		case KEY_F(2): *byte = 137;  break;
		case KEY_F(3): *byte = 134;  break;
		case KEY_F(4): *byte = 138;  break;
		case KEY_F(5): *byte = 135;  break;     
		case KEY_F(6): *byte = 139;  break;
		case KEY_F(7): *byte = 136;  break;
		case KEY_F(8): *byte = 140;  break;
		case KEY_F(12): *byte = 0;   break;
	
		case '`':	*byte = 95;  break;				
		
		case 10 : break; 
		case '[': break;
		case ']': break;
		case '^': break;
		case '&': break;
			  
		default:
			if ((*byte >= 'a') && (*byte <= 'z')) *byte = *byte ^ 0x20;
			else if ((*byte >= 'A') && (*byte <= 'Z')) *byte = *byte^ 0x20;
			     else if ((*byte < ' ') || (*byte > '@')) *byte = -1;
	}
}


/////////////////////////////////
//  Print to the ncurses console
/////////////////////////////////
//
void
screen_print(unsigned char b)
{ 
	// Handle quotation mark
	//
	if (b == '"') quotation = !quotation;
	if ((quotation) && (b == 10)) quotation = 0;
	if ((quotation) && ((b == 145) || (b == 29) || (b == 157) || (b == 17) || (b == 148) || (b == 19) || (b == 147) || ((b > 132) && (b < 141)))) b = '|';
	
	switch(b)
	{
		// Delete
		case 20:
			if (win->_curx > 0) win->_curx--;
			else if (win->_cury > 0) { win->_cury--; win->_curx = 39;}
			if (win->_curx != 39) wdelch(win);
			break;

		// Move up
		case 145:
			if (win->_cury > 0) win->_cury--;
			break;
	    
		// Move right
		case 29:
			if (win->_curx < 39) win->_curx++;
			else if (win->_cury < 24) { win->_cury++; win->_curx = 0;}
				else {scroll(win); win->_curx = 0;}
			break;
	    
		// Move left
		case 157:
			if (win->_curx > 0) win->_curx--;
			else if (win->_cury > 0) { win->_cury--; win->_curx = 39;}
	      		break;	
	      
		// Move down
		case 17:
			if (win->_cury < 24) win->_cury++;
			else scroll(win);
			break;
	    
		// Return
		case 10:
			if (win->_cury < 24) win->_cury++;
			else scroll(win);
			win->_curx = 0;
			break;
	      
	    	// Insert
		case 148:
			mvwinsch(win, win->_cury, win->_curx, ' ');
			break;
	    
		// Home
		case 19:
			win->_curx = 0;
			win->_cury = 0;
			break;
	    
		// Clear
		case 147:
			wclear(win);
			break;
	    
		// Another character
		default:
			if (((b < 133) || (b > 140)) && (b > 26)) 
			{
				wprintw(win, "%c", b);
			}
	}
	
	// Refresh the window
	wrefresh(win);
}


///////////////////
//    M A I N
///////////////////
//
int
main (int argc, char ** argv)
{
	// Variables
	//
	int b = 147;  /* Clear */
	struct pollfd polls[2];
	char ccons[] = "/dev/ccons";
	char * device_name;
	
	// Arguments
	//
	if (argc == 2) device_name = argv[1];
	else	if (argc == 1) device_name = ccons;
		else
		{
			printf("Usage: console [device name]\n");
			exit(1);
		}
	
	// Open device
	//
	device_in = open(device_name, O_WRONLY);
	device_out = open(device_name, O_RDONLY);
	
	if ((device_in == -1) || (device_out == -1))
	{	
		printf("Unable to open device %s.\n", device_name);
		exit(1);
	}

	// Don't care about "READY." (return from SYS)
	// So close and reopen device_out
	//
	close(device_out);
	sleep(1);
	device_out = open(device_name, O_RDONLY);
	
	if (device_out == -1)
	{
		printf("Unable to open device %s.\n", device_name);
		exit(1);
	}
	
	// Build up the poll structures
	//
	polls[0].fd = 0;
	polls[0].events = POLLIN;
	polls[0].revents = 0;
	
	polls[1].fd = device_out;
	polls[1].events = POLLIN;
	polls[1].revents = 0;
	
	// Init the console
	//
	InitCurses();
	wrefresh(win);
	
	// Clear C64's display
	write(device_in, &b, 1);
	
	// Main loop
	//
	while (b != KEY_F(10))
	{
		// Wait for input
		poll(polls, 2, -1);
		
		// Input from the device
		if(polls[1].revents == POLLIN) 
		{
			 // Read from the device
			 read(device_out, &b, 1);
			 
			 //Print to the screen
			 screen_print(b);
		}
		
		// Input from stdin
		if(polls[0].revents == POLLIN)
		{
			// Read from stdin
			b = wgetch(win);
				
			// Exit on F10
			if (b == KEY_F(10)) continue;
			
			// Convert character
			convert(&b);

			// -1: Do not write
			if (b == -1) continue;

			// Write to device
			write(device_in, &b, 1);

			// Local echo
			if (b != 10) screen_print((unsigned char)b);	
		}

	} 

	// Close devices
	//
	close (device_in);
	close (device_out);

	// Restore console
	//
	delwin(win);
	initscr();
	refresh();
	endwin();

	// Exit
	exit(0);
}

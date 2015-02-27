/* *********************************************
 *  
 *  c64_console.c v1.20
 *  
 *  C64_CONSOLE
 *  Linux kernel module for connecting C64 & PC
 *	
 *  Kernel version 2.4.x
 *
 * *********************************************
 *  Lohner Roland, 2003.
 * **********************
 */


/////////////////////////////////
//  Deal with MODVERSIONS first
/////////////////////////////////
//
#include <linux/config.h>

#if CONFIG_MODVERSIONS == 1
#define MODVERSIONS
#include <linux/modversions.h>
#endif


///////////////////////////////
//  Standard in kernel modules
///////////////////////////////
//
#include <linux/version.h>   /* Kernel version */
#include <linux/kernel.h>    /* We're doing kernel work */
#include <linux/module.h>    /* Specifically, a module */


///////////////////////////////////
//  Standard for character devices
///////////////////////////////////
//
#include <linux/fs.h>       /* The character device 
                             * definitions are here */
#include <linux/wrapper.h>  /* A wrapper for compatibility
                             * issues */

/////////////////////////////
//  Another required inclues
/////////////////////////////
//
#include <asm/uaccess.h>    /* For put_user and get_user */
#include <asm/io.h>         /* For outb and inb */

#include <linux/interrupt.h>/* We want to use interrupts */
#include <linux/sched.h>    /* We also need the ability to */ 
#include <linux/wait.h>     /* put ourselves to sleep
                             * and wake up later */
#include <linux/delay.h>    /* For udelay */

#include <linux/circ_buf.h> /* We use a circular buffer, too */
#include <linux/poll.h>	    /* For polling */


///////////////////////////
//  Defines
///////////////////////////
//
// The name for our device, as it will appear in /proc/devices
#define DEVICE_NAME "c64_console"

// Major device number
#define DEVICE_NUM 248  /* 248: free for experimental use */ 

// Possible states of a transactions
//
#define TRANSFER    1	/* Waiting for C64 to receive the char */
#define WAIT        2	/* Waiting for C64 to process data */
#define INTERRUPTED 3	/* Transaction interrupted */
#define SUCCESS     0	/* Transaction OK */

// Timeout and delay time settings
//
#define RX_TIMEOUT_US 100 /* Timeout by receive in microsecs */

#define TRANSFER_US 250   /* Wait for C64 to receive data (in us) */

#define COMM_DELAY_MS 30  /* Wait for C64 to process data (in ms) */
#define COMM_DELAY_CYC ((COMM_DELAY_MS * HZ) / 1000)

// Sizes
//
#define BUF_SIZE 1024     /* Size of the circular buffer (only 2^n) */


///////////////////////////
//  Global variables
///////////////////////////
//
static int __initdata io = 0x278;  /* Default parallel port base address */
static int __initdata irq = 7;     /* Default parallel port irq */

static int major;		/* Major device number */

static int state = SUCCESS;	/* Current state of transaction */
static int irq_state;		/* Was interrupt requested? */

static wait_queue_head_t WaitQ; /* Waitqueue used to go sleep */
static wait_queue_head_t ReadQ; /* Waitqueue used if buffer is empty */

static unsigned char array[BUF_SIZE];	/* The circular buffer */
static unsigned char received;		/* The byte received from C64 */
static int us;				/* Microsec counter by Rx */
static int head, tail;			/* Head and tail position
					 * in the circular buffer */
static int Device_Open_For_Reading = 0;	/* Is the device open for reading right now? */
static int Device_Open_For_Writing = 0; /* Is the device open for writing right now? */


///////////////////////
//  Module parameters
///////////////////////
//
MODULE_DESCRIPTION("Module for C64 PC console.");
MODULE_AUTHOR("Roland Lohner <loci@sch.bme.hu>");
MODULE_LICENSE("GPL");

MODULE_PARM(io, "i");
MODULE_PARM(irq, "i");

MODULE_PARM_DESC(io, "Set I/O base of paralell port. (default: 0x278)");
MODULE_PARM_DESC(irq, "Set IRQ of paralell port. (default: 7)");


/* *************** Specific Declarations *************** */


/////////////////////////////////////
//  Convert newline to carriage back
/////////////////////////////////////
//
static void 
convert_newline (unsigned char* ch)
{
	if (*ch == 13) { *ch = 10; return; }
	if (*ch == 10)   *ch = 13;
}


//////////////////////////
//  Read byte from buffer
//////////////////////////
//
static unsigned char 
outbuf (unsigned char* success)
{
	unsigned char value;
	
	// Wait if buffer is empty
	//
	if (CIRC_CNT(head, tail, BUF_SIZE) < 1)
	{
		interruptible_sleep_on( & ReadQ);
	}
	
	// Report error, if interrupted
	// 
	if (CIRC_CNT(head, tail, BUF_SIZE) < 1)
	{
		if (success) *success = 0;
		return 0;
	}
	
	// Get byte
	value = array[tail];
 
	// Handle buffer
	tail++;
	if (tail == BUF_SIZE) tail = 0;
	
	// Return
	if (success) *success = 1;
	return value;
}


/////////////////////////
//  Write byte to buffer
/////////////////////////
//
static void 
inbuf (unsigned char value)
{
	// Convert newline to carriage back
	convert_newline( & value);
	
	// Make space if buffer is full
	while (CIRC_SPACE(head, tail, BUF_SIZE) < 1) 
	{
		outbuf(NULL);
	}

	// Put byte
	array[head] = value;
	
	// Handle buffer
	head++;
	if (head == BUF_SIZE) head = 0;

	// Wake up buffer readers
	wake_up_interruptible( & ReadQ);
}


////////////////////////////////////
//  Paralell port interrupt handler
////////////////////////////////////
//
static void 
irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{	
	// Do not interrupt a transfer. Set irq_state instead.
	if (state == TRANSFER) 
	{
		irq_state = 1;
		return;
	}
	
	// Receice from C64 //
	
	// Switch input mode, set INIT
	outb (0x34, io + 2);
	
	// Create falling edge on INIT -> FLAG
	udelay(1);
	outb (0x30, io + 2);
			
	// Wait cycle - wait for C64 to put the byte on the bus
	us = 0;
	received = 0xFF;
	while ((received == 0xFF) && (us < RX_TIMEOUT_US))
	{
		us++;
		received = inb(io);
		udelay(1);
	}
	
	// Store the value read in
	if (Device_Open_For_Reading > 0) inbuf(inb(io));
			
	// Set INIT
	outb (0x34, io + 2);
}


///////////////////////
//  Send a byte to C64
///////////////////////
//
static int 
send(unsigned char b)
{
	// Do not send 0xFF (see protocol)
	if (b == 0xFF) return SUCCESS;

	// Convert newline to carriage back
	convert_newline(&b);
	
	// Reset irq_state, set status
	irq_state = 0;
	state = TRANSFER;
	
	// Write byte to parallel port
	outb (b, io);

	// Clear PC's INIT (CIA chip detects the falling edge) & switch forward mode
	outb (0x10, io + 2);
	
	// Wait for C64 to receive the byte
	udelay(TRANSFER_US);

	// Switch input mode & set INIT
	outb (0x34, io + 2);
	
	// Update status
	state = WAIT;

	// If interrupt was requested, call irq_handler
	if (irq_state > 0)
	{
		/* Disable interrupts */
		cli();
		
		/* Call handler */
		irq_handler(irq, NULL, NULL);
		
		/* Enable interrupts */
		sti();
	}
 
	// Wait for C64 to process data
	if (interruptible_sleep_on_timeout( & WaitQ, COMM_DELAY_CYC) != 0)
	{
		state = INTERRUPTED;
	}
	else
	{
		state = SUCCESS;
	}

	return state;
}


/* *************** Device Declarations *************** */


///////////////////////
//  Opening the device
///////////////////////
//
static int 
device_open (struct inode* inode, struct file* file)
{
	// Type of the access
	int mode = (file->f_flags & O_ACCMODE);
	
	// Reading
	if ((mode == O_RDONLY) || (mode == O_RDWR))
	{
		// If device is already open for reading send error
		if (Device_Open_For_Reading > 0) return -EBUSY;
		Device_Open_For_Reading++;
		
		// Reset the circular buffer
		head = 0;
		tail = 0;
		
		// Init the reader wait queue
		init_waitqueue_head ( & ReadQ);
	}
		
	// Writing
	if ((mode == O_WRONLY) || (mode == O_RDWR))
	{
		// If device is already open for writing send error
		if (Device_Open_For_Writing > 0) return -EBUSY;
		Device_Open_For_Writing++;

		// Init the handshaking wait queue
		init_waitqueue_head ( & WaitQ);
	}
	
	// Increment usage counters
	MOD_INC_USE_COUNT;
	
	return SUCCESS;
}


///////////////////////
//  Closing the device
///////////////////////
//
static int 
device_release(struct inode* inode, struct file* file)
{
	int mode = (file->f_flags & O_ACCMODE);

	// Decrement usage counters
	// 
	if ((mode == O_RDONLY) || (mode == O_RDWR)) Device_Open_For_Reading--;
	if ((mode == O_WRONLY) || (mode == O_RDWR)) Device_Open_For_Writing--;			
	MOD_DEC_USE_COUNT;

	return SUCCESS;
}


///////////////////////
//  Polling the device
///////////////////////
//
static unsigned int
device_poll(struct file* file, struct poll_table_struct* poll_table)
{
	unsigned int mask = 0;

	poll_wait(file, & ReadQ, poll_table);

	if (CIRC_CNT(head, tail, BUF_SIZE) > 0) mask |= POLLIN | POLLRDNORM; /* readable */
	mask |= POLLOUT | POLLWRNORM; /* writable */

	return mask;
}


///////////////////////
//  Reading the device
///////////////////////
//
static ssize_t 
device_read(struct file* file,
			char* buffer,    /* The buffer to fill with data */
			size_t length,   /* The length of the buffer */
			loff_t* offset ) /* Our offset in the file */
{  
	unsigned char ch;
	unsigned char success;
	ssize_t read = 0;

	// Reader loop
	// Read at least one character
	while (((CIRC_CNT(head, tail, BUF_SIZE) > 0) || (read == 0)) && (length > 0))
	{
		// Read character from our circular buffer
		ch = outbuf(& success);
		
		// If the procedure was interrupted, send error
		if (success != 1) return -EINTR;
		
		// Put the character read to the user space
		put_user(ch, buffer++);
		
		// Handle indexes
		length--;
		read++;
	}

	// Return number of bytes read
	return read;
}


///////////////////////
//  Writing the device
///////////////////////
//
static ssize_t 
device_write(struct file* file,
			 const char* buffer,	/* The buffer */
			 size_t length,		/* The length of the buffer */
			 loff_t* offset)	/* Our offset in the file */
{
	unsigned char ch;
	ssize_t written = 0;
	
	// Transfer loop
	// Send all the bytes in the file buffer
	while (length > 0)
	{	
		// Get the byte from the user space
		get_user(ch, buffer++);
		
		// Stop if interrupted
		if (send (ch) == INTERRUPTED) return -EINTR;
		
		// Handle indexes
		length--;
		written++;
	}
		
	// Return number of bytes written
	return written;
}


/* *************** Module Declarations *************** */


// File operation structure (global) 
static struct file_operations Fops = {
				read    : device_read, 
				write   : device_write,
				open    : device_open,
				release : device_release,
				poll    : device_poll
			      };


//////////////////////////
//  Initialise the module
//////////////////////////
//
int 
init_module(void)
{
	int result;
	
	// Switch the ECP paralell port to byte mode
	outb(0x20, io + 0x402);
	
	// Request paralell port IRQ (5 or 7)
	result = request_irq(
				irq,		/* The number of the paralell port IRQ */
				irq_handler,	/* Our interrupt handler */
				SA_INTERRUPT,	/* Options: Fast interrupt handler */
				DEVICE_NAME,	/* Name in /proc/interrupts */
				NULL
			    );

	if (result != SUCCESS)
	{
		printk ("%s: Requesting of IRQ %d failed with %d.\n", DEVICE_NAME, irq, result);
		return result;
	}
	 
	// Set the paralell port to input mode and enable interrupt and set INIT
	// (First write 0x14 to the port: on my HW an interrupt occurs (no idea, why..))
	outb (0x14, io + 2);
	outb (0x34, io + 2);
	
	// Call irq handler (Get byte in case C64 is waiting for PC to receive)
	udelay(35);
	irq_handler(irq, NULL, NULL);
	
	// Register the character device (at least try ;-)
	major = register_chrdev( DEVICE_NUM,	/* Major device num */
	                         DEVICE_NAME,	/* Name */
	                         & Fops );	/* File ops struct */

	// Register OK: major = DEVICE_NUM
	if (major == 0) major = DEVICE_NUM;
	
	// Negative return values signify an error
	if (major < 0) 
	{
		printk ("%s: Device registering failed with %d.\n", DEVICE_NAME, major);
	
		// Swith paralell port to input mode and disable IRQ
		outb (0x24, io + 2); 
		
		// Unregister IRQ
		free_irq(irq, NULL);

		return major;
	}

	// OK
	printk ("%s: Registration OK. Major device number is %d.\n", DEVICE_NAME, major);
	return SUCCESS;
}


////////////////////////
//  Clean up the module
////////////////////////
//
void 
cleanup_module(void)
{
	int result;
	
	// Swith paralell port to input mode and disable IRQ
	outb (0x24, io + 2);       

	// Unregister IRQ
	free_irq(irq, NULL);

	// Unregister the device 
	result = unregister_chrdev(major, DEVICE_NAME);
	
	// If there's an error, report it
	if (result < 0) printk("%s: Error by unregistering device. (%d)\n", DEVICE_NAME, result);
} 


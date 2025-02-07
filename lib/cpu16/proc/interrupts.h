// General & I/O
#define INT_EXIT         0x00 // exit code from stack
#define INT_READ         0x01 // push char to stack
#define INT_WRITE        0x02 // write char from stack
#define INT_DATE         0x03 // get date to dx register
#define INT_RESET        0x04 // reset the CPU

// Videobuffer
#define INT_VIDEO_WRITE  0x0C // set vga[si] to ax
#define INT_VIDEO_FLUSH  0x11 // flush the videobuffer
#define INT_RAND         0x21 // get random number to dx
#define INT_WAIT         0x22 // wait dx milliseconds

#ifndef MACROS_H
#define MACROS_H

#define DEBUG_MODE

//#define OS_DEBUG

//#define DEBUG_PINS

// use these for functions which return a failure/success status
typedef enum { FAILURE, SUCCESS } SF_STATUS;

int StartCritical(void);
void EndCritical(int);

#ifndef ASSERT
#define ASSERT(condition) if(!(condition)) { return (FAILURE); }
#endif

#ifndef NULL
#define NULL 0
#endif

// Ceiling macro that returns an integer
#define	INTCEIL(x) ( ((x)-(int)(x)) > 0 ? ((int)(x)+1) : (int)(x) )
#define MAX(x,y) ((x)<(y)?(y):(x))
#define MIN(x,y) ((x)>(y)?(y):(x))
#define ABS(expr) ((expr) >= 0? (expr) : -(expr))


// #ifndef TRUE
// #define TRUE 1
// #endif

// #ifndef FALSE
// #define FALSE 0
// #endif

// ** Debugging tools ** //
#define PF0 								(*(	(volatile unsigned long *)(0x40025004)))
#define PC4									(*( (volatile unsigned long *)(0x40006040)))
#define PC5									(*( (volatile unsigned long *)(0x40006080)))
#define PC6									(*( (volatile unsigned long *)(0x40006100)))
#define PC7									(*( (volatile unsigned long *)(0x40006200)))
#define LED_TOGGLE() 				{PF0 ^= 0x01;}
#define LED_OFF()		 				{PF0 =	0;}
#define LED_ON()		 				{PF0 = 0x01;}
#define PC7_TOGGLE()				{PC7 ^= 0x80;}
#define PC6_TOGGLE()				{PC6 ^= 0x40;}
#define PC5_TOGGLE()				{PC5 ^= 0x20;}
#define PC4_TOGGLE()				{PC4 ^= 0x10;}
#define PC4_ON()						{PC4 = 0x10;}
#define PC5_ON()						{PC5 = 0x20;}
#define PC6_ON()						{PC6 = 0x40;}
#define PC7_ON()						{PC7 = 0x80;}
#define PC4_OFF()						{PC4 = 0;}
#define PC5_OFF()						{PC5 = 0;}
#define PC6_OFF()						{PC6 = 0;}
#define PC7_OFF()						{PC7 = 0;}

// typedefs for common types
typedef signed int		INT;
typedef unsigned int	UINT;

/* These types are assumed as 8-bit integer */
typedef signed char		CHAR;
typedef unsigned char	UCHAR;
typedef unsigned char	BYTE;

/* These types are assumed as 16-bit integer */
typedef signed short	SHORT;
typedef unsigned short	USHORT;
typedef unsigned short	WORD;

/* These types are assumed as 32-bit integer */
typedef signed long		LONG;
typedef unsigned long	ULONG;
typedef unsigned long	DWORD;

/* Boolean type */
typedef enum { FALSE = 0, TRUE } BOOL;

#endif

#include "io/rit128x96x4.h"
#include "thread/thread.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "inc/lm3s8962.h"
#include "thread/systick.h"
#include "io/adc.h"
#include "util/os.h"
#include <stdio.h>
#include <string.h>
//#include "labs/Lab4.h"
#include "io/file.h"

#define NUM_COMMANDS 14
#define MAX_COMMAND_NAME_LENGTH 10
#define MAX_COMMAND_ARGUMENTS 5
#define MAX_ARGUMENT_LENGTH 20
#define MAX_COMMAND_USAGE_LENGTH 50
#define MAX_COMMAND_DESCRIPTION_LENGTH 50

typedef struct {
	char name[MAX_COMMAND_NAME_LENGTH];
	char description[MAX_COMMAND_DESCRIPTION_LENGTH];
	char usage[MAX_COMMAND_USAGE_LENGTH];
	void (*command)(int argc, char** args);
} Command;

static Command commands[NUM_COMMANDS];
static Command unknownCommand;
static UINT numCommands = 0;

/*
 * adds a command to the terminal interface
 * name - the name of the command. this is how it will be called
 * function - the callback which handles this command
 * description - the short text description about what the command does
 * usage - the usage of the command, gives a description of its parameters
 */
static void addCommand(const char* name, void (*function)(char** args), 
		const char* description, const char* usage) {
	if(numCommands < NUM_COMMANDS) {
		strncpy(commands[numCommands].name, name, MAX_COMMAND_NAME_LENGTH);
		strncpy(commands[numCommands].description, description, MAX_COMMAND_DESCRIPTION_LENGTH);
		strncpy(commands[numCommands].usage, usage, MAX_COMMAND_USAGE_LENGTH);
		commands[numCommands].command = function;

		numCommands++;
	}
}

/*
 * looks up and returns a pointer to a command by name
 * if that command is not present, it returns the unknown command
 *
 * commandName - null terminated string to match
 * len - commandName length
 * 
 * return - the pointer to the specified command or &unknownCommand
 */
static Command* getCommand(char* commandName) {
	int i;
	size_t commandNameLen = strlen(commandName);
	size_t length = commandNameLen<MAX_COMMAND_NAME_LENGTH?commandNameLen:MAX_COMMAND_NAME_LENGTH;

	for(i=0;i<numCommands;i++) {
		if(!strncmp(commandName, commands[i].name, length)) {
			return &commands[i];
		}
	}

	return &unknownCommand;
}

/*
 * breaks up the raw string into several null terminated strings
 * 
 * raw - pointer to the null terminated raw string
 * args - array of char* s to populate with pointers to the various strings
 *
 * return - the number of arguments found, or -1 if there were too many arguments
 */
static UINT argumentify(char* raw, char** args) {
	UINT argCount = 0;

	while(*raw != '\0') {
		// skip any whitespace
		if(*raw == ' ' || *raw == '\t') {
			raw++;
			continue;
		}

		// make sure we aren't going over the argument limit
		if(argCount == MAX_COMMAND_ARGUMENTS) {
			return -1;
		}

		// add the new argument to the args list
		args[argCount++] = raw;
		raw++;

		// go to the next chunk of whitespace ( read through all chars in the argument )
		while(*raw != '\0' && *raw != ' ' && *raw != '\t') {
			raw++;
		}

		// if we aren't at the end of the string yet,
		// put a null terminator to break the arguments
		// into separate strings. This makes them easier
		// to handle than one continuous string.
		if(*raw != '\0') {
			*raw = '\0';
			raw++;
		}
	}

	return argCount;
}

/*
 * interprets and executes the command
 * 
 * command - command to execute
 */
static void interpret(char* command) {
	char* startPtr = command;
	char* commandArguments[MAX_COMMAND_ARGUMENTS];
	Command* commandPtr = NULL;
	UINT numArgs = 0;

	while(*command != ' ' && *command != '\t' && *command != '\0') {
		command++;
	}

	if(*command != '\0') {
		*command = '\0';
		command++;

		numArgs = argumentify(&command[0], commandArguments);
	}

	commandPtr = getCommand(startPtr);
	commandPtr->command(numArgs, commandArguments);


}

static void echo(int argc, char** args) {
	if(argc == 1) {
		printf("%s\n\r", args[0]);
		
		file_redirect_start("redirect", strlen("redirect"));
		
		printf("%s\n\r", args[0]);
		
		file_redirect_end();
	}
}

static void help(int argc, char** args) {
	int i;
	printf("\n\rPossible commands: \n\r");

	for(i=0;i<numCommands;i++) {
		printf("    %s - %s\n\r", commands[i].name, commands[i].description);
	}

	printf("\n\r");
}

static void unknown(int argc, char** args) {
	printf("Command not recognized. Type \"help\" for possible commands\n\r");
}

static void everySecondPrint(void) {
	static int i = 0;

	if(i++ == 9) {
		printf("Current time in cycles: %ul\n\r", OS_Time());
		i = 0;
	}
}

static void timer(int argc, char** args) {
	if(argc >= 1) {
		if(!strncmp(args[0], "get", 3)) {
			printf("%ul ms\n\r", OS_Time());
		}
		else if(!strncmp(args[0], "clear", 5)) {
			OS_ClearTime();
		}
		else if(!strncmp(args[0], "periodic", 8)) {
			if(argc == 2) {
				if(!strncmp(args[1], "start", 5)) {
					OS_AddPeriodicThread(everySecondPrint, 100, 0);
				}
				else if(!strncmp(args[1], "cancel", 6)) {
					OSRemovePeriodicThread(everySecondPrint);
				}
			}
		}
	}
}

static void usage(int argc, char** args) {
	Command* commandPtr;

	if(argc == 1) {
		commandPtr = getCommand(args[0]);
		printf("\n\r");
		printf("    %s %s", commandPtr->name, commandPtr->usage);
		printf("\n\r\n\r");
	}
}

static void oled(int argc, char** args) {
	if(argc >= 1) {
		if(!strncmp(args[0], "print", strlen("print"))) {
			if(argc == 2) {
				RIT128x96x4StringDraw(args[1], 0, 0, 15);
			}
		}
		if(!strncmp(args[0], "clear", strlen("clear"))) {
			RIT128x96x4Clear();
		}
	}
}

static void doNothing(void) {
}

///
// Right now just reads in from ADC0
// and prints to serial stream
///
static void adc(int argc, char** args) {
	USHORT adcRead;
	USHORT intInput;

	switch(argc)
	{
		case 0: // Just read from channel 0
			printf("ADC_0: %u\r\n",ADC_In(0));
			break;
		case 1: // Read from specified channel number
			if(!strncmp(args[0],"filter",strlen("filter"))) { // Toggle filter
				//if(ToggleFilter()) printf("Filter Enabled\r\n");
				//else printf("Filter Disabled\r\n");
			}
			else if(!strncmp(args[0],"fft",strlen("fft"))) {
				//PrintFFT();
			}
			else if(!strncmp(args[0],"volt",strlen("v"))) {
				//PrintVoltage();
			}
			else {
				intInput = (USHORT) strtol(args[0],NULL,10);
				printf("ADC_%u: %u\r\n",intInput,ADC_In(intInput));
			}
			break;
		case 2: // Set ADC trigger type
			if(!strncmp(args[0],"trig",strlen("trig"))) {
				if(!strncmp(args[1],"oneshot",strlen("oneshot"))) {
					//SetTrigger(ADC_TRIG_ONESHOT);
					printf("ADC Trigger set to ONESHOT type\r\n");
				}
				else if(!strncmp(args[1],"always",strlen("always"))) {
				//	SetTrigger(ADC_TRIG_ALWAYS);
					printf("ADC Trigger set to ALWAYS type\r\n");
				}
			}
			break;
		default:
			break;
	}
}

static void InterruptTimer(int argc, char** args) {
	if(!strncmp(args[0],"clear",strlen("clear")))
		ClearInterruptTimer();
	else if(!strncmp(args[0],"stats",strlen("stats")))
		PrintInterruptTimerStats();
}

static void plotmode(int argc, char** args) {
	if(!strncmp(args[0],"fft",strlen("fft"))) {
		//SetPlotMode(PLOTMODE_FFT);
	}
		
	else if(!strncmp(args[0],"volt",strlen("volt"))) {
		//SetPlotMode(PLOTMODE_VOLT);
	}
}

#define READSIZE 16
file_t tempFile;
UCHAR ReadBuf[READSIZE];

static void rm(int argc, char** args) {
	if(file_open(args[0],strlen(args[0]),&tempFile) == SUCCESS) {
		file_rm(&tempFile);
	}
}

static void touch(int argc, char** args) {
	if(file_open(args[0], strlen(args[0]), &tempFile) == SUCCESS) { return; }
	file_touch(args[0],strlen(args[0]));
}

static void ls(int argc, char** args) {
	file_ls();
}

static void writex(int argc, char** args) {
	int repeat = atoi(args[1]);
	
	if(file_open(args[2],strlen(args[2]),&tempFile) != SUCCESS) {
		file_touch(args[2],strlen(args[2]));
	}
	file_open(args[2],strlen(args[2]),&tempFile);
	printf("repeating %s %d times\n\r", args[0], repeat);
	
	while(repeat--) {
		file_append(&tempFile, args[0], strlen(args[0]));
	}
}

static void write(int argc, char** args) {
	if(file_open(args[1],strlen(args[1]),&tempFile) != SUCCESS) {
		file_touch(args[1],strlen(args[1]));
		file_open(args[1],strlen(args[1]),&tempFile);
		file_write(&tempFile,args[0],strlen(args[0]));
	}
	else {
		file_append(&tempFile,args[0],strlen(args[0]));
	}
}

static void cat(int argc, char** args) {
	int readCnt,i=0;
	
	file_open(args[0],strlen(args[0]),&tempFile);
	
	do {
		readCnt = file_read(&tempFile,ReadBuf,sizeof(UCHAR)*(READSIZE-1));
		
		ReadBuf[readCnt] = '\0';
		
		printf("%s", ReadBuf);
	} while(readCnt);
	
	printf("\r\n");
}

static void format(int argc, char** args) {
	printf("Formatting disk of size %d...",FILESYSTEM_BYTESIZE);
	if(fs_format() == SUCCESS) { fs_init(); printf("Done.\r\n"); }
	else { printf("Formatting Failed!\r\n"); }
	
}

static void ps(int argc, char** args) {
	list_t *processes = ActiveTCBList();
	TCB *tcb = (TCB*)currentThread();
	int i;
	
	printf("TID\tPriority\r\n");
	for(i=0;i<list_size(processes);i++) {
		printf("%d\t%d\n\r", tcb->id,tcb->priority);
		tcb = (TCB*)list_next(tcb);
	}
}

void Interpreter(void) {
	char commandInput[100];

	unknownCommand.command = unknown;

	addCommand("echo", echo, "prints text to the console", "TEXT");
	addCommand("oled", oled, "manipulate the oled display", "(print TEXT | clear)");
	addCommand("timer", timer, "manipulate the timer functions", "get | clear | (periodic (start | cancel))");
	addCommand("help", help, "prints a list of commands", "");
	addCommand("adc", adc, "Get a single ADC reading","CHAN_NUM");
	addCommand("usage", usage, "print the usage of a command", "COMMAND");
	//addCommand("int_timer",InterruptTimer,"manipulate int timer", "clear | stats");
	//addCommand("plotmode",plotmode,"Change plot mode", "fft | volt");
	
	addCommand("format",format,"format the disk", "format");
	addCommand("cat",cat,"list the contents of a file", "cat filename");
	addCommand("write",write,"write to file", "write [text] filename");
	addCommand("writex",writex,"specify a multiplier to the text", "writex [text] [integer] filename");
	addCommand("touch",touch,"create an empty file","touch filename");
	addCommand("ls",ls,"list directory","ls");
	addCommand("rm",rm,"remove a file","rm filename");
	
	addCommand("ps",ps,"List active processes","");
	
	/*
	   To add your own command: 
	   1) make sure to modify the NUM_COMMANDS field above. Just add 1 to it for every new command
	   2) create a function that takes parameters (int, char**) as shown above.
	   3) put another addCommand function call as shown above
	 */

	while(1) {
		printf("$ ");
		fgets(commandInput, sizeof(commandInput), NULL);
		interpret(commandInput);
	}
}


; <<< Use Configuration Wizard in Context Menu >>>
;******************************************************************************
;
; Startup.s - Startup code for Stellaris.
;
; Copyright (c) 2006-2008 Luminary Micro, Inc.  All rights reserved.
; 
; Software License Agreement
; 
; Luminary Micro, Inc. (LMI) is supplying this software for use solely and
; exclusively on LMI's microcontroller products.
; 
; The software is owned by LMI and/or its suppliers, and is protected under
; applicable copyright laws.  All rights are reserved.  You may not combine
; this software with "viral" open-source software in order to form a larger
; program.  Any use in violation of the foregoing restrictions may subject
; the user to criminal sanctions under applicable laws, as well as to civil
; liability for the breach of the terms and conditions of this license.
; 
; THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
; OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
; LMI SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
; CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
; 
; This is part of revision 2523 of the Stellaris Peripheral Driver Library.
;
;******************************************************************************

;******************************************************************************
;
; <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
;
;******************************************************************************
Stack   EQU     0x00002000

;******************************************************************************
;
; <o> Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
;
;******************************************************************************
Heap    EQU     0x00000000

;******************************************************************************
;
; Allocate space for the stack.
;
;******************************************************************************
        AREA    STACK, NOINIT, READWRITE, ALIGN=3
StackMem
        SPACE   Stack
__initial_sp

;******************************************************************************
;
; Allocate space for the heap.
;
;******************************************************************************
        AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
HeapMem
        SPACE   Heap
__heap_limit

;******************************************************************************
;
; Indicate that the code in this file preserves 8-byte alignment of the stack.
;
;******************************************************************************
        PRESERVE8

;******************************************************************************
;
; Place code into the reset code section.
;
;******************************************************************************
        AREA    RESET, CODE, READONLY
        THUMB

;******************************************************************************
;
; External declarations for the interrupt handlers used by the application.
;
;******************************************************************************
		EXTERN  Timer0BIntHandler
		EXTERN  Timer1AIntHandler
		EXTERN  Timer1BIntHandler
		EXTERN  Timer2AIntHandler
		EXTERN  Timer2BIntHandler
		EXTERN  UARTIntHandler
		;EXTERN  SysTickHandler
		EXTERN  GPIOPortFHandler
		EXTERN	ADC0_SS3Handler
		EXTERN 	ADC0_SS1Handler
		EXTERN	GPIOPortEHandler
		EXTERN 	Ethernet_Handler
		EXTERN	Timer3A_Handler
		EXTERN	Timer3B_Handler
		
;******************************************************************************
;
; The vector table.
;
;******************************************************************************
        EXPORT  __Vectors
__Vectors
        DCD     StackMem + Stack            ; Top of Stack
        DCD     Reset_Handler               ; Reset Handler
        DCD     NmiSR                       ; NMI Handler
        DCD     FaultISR                    ; Hard Fault Handler
        DCD     IntDefaultHandler           ; MPU Fault Handler
        DCD     IntDefaultHandler           ; Bus Fault Handler
        DCD     IntDefaultHandler           ; Usage Fault Handler
        DCD     0                           ; Reserved
        DCD     0                           ; Reserved
        DCD     0                           ; Reserved
        DCD     0                           ; Reserved
        DCD     IntDefaultHandler           ; SVCall Handler
        DCD     IntDefaultHandler           ; Debug Monitor Handler
        DCD     0                           ; Reserved
        DCD     PendSVHandler               ; PendSV Handler
        DCD     SysTickHandler              ; SysTick Handler
        DCD     IntDefaultHandler           ; GPIO Port A
        DCD     IntDefaultHandler           ; GPIO Port B
        DCD     IntDefaultHandler           ; GPIO Port C
        DCD     IntDefaultHandler           ; GPIO Port D
        DCD     GPIOPortEHandler           ; GPIO Port E
        DCD     UARTIntHandler              ; UART0
        DCD     IntDefaultHandler           ; UART1
        DCD     IntDefaultHandler           ; SSI
        DCD     IntDefaultHandler           ; I2C
        DCD     IntDefaultHandler           ; PWM Fault
        DCD     IntDefaultHandler           ; PWM Generator 0
        DCD     IntDefaultHandler           ; PWM Generator 1
        DCD     IntDefaultHandler           ; PWM Generator 2
        DCD     IntDefaultHandler           ; Quadrature Encoder
        DCD     IntDefaultHandler           ; ADC Sequence 0
        DCD     ADC0_SS1Handler	            ; ADC Sequence 1
        DCD     IntDefaultHandler           ; ADC Sequence 2
        DCD     ADC0_SS3Handler	            ; ADC Sequence 3
        DCD     IntDefaultHandler           ; Watchdog
        DCD     IntDefaultHandler           ; Timer 0A
        DCD     Timer0BIntHandler           ; Timer 0B
        DCD     Timer1AIntHandler           ; Timer 1A
        DCD     Timer1BIntHandler           ; Timer 1B
        DCD     Timer2AIntHandler           ; Timer 2A
        DCD     Timer2BIntHandler           ; Timer 2B
        DCD     IntDefaultHandler           ; Comp 0
        DCD     IntDefaultHandler           ; Comp 1
        DCD     IntDefaultHandler           ; Comp 2
        DCD     IntDefaultHandler           ; System Control
        DCD     IntDefaultHandler           ; Flash Control
        DCD     GPIOPortFHandler            ; GPIO Port F
        DCD     IntDefaultHandler           ; GPIO Port G
        DCD     IntDefaultHandler           ; GPIO Port H
        DCD     IntDefaultHandler           ; UART2 Rx and Tx
        DCD     IntDefaultHandler           ; SSI1 Rx and Tx
        DCD     Timer3A_Handler           	; Timer 3 subtimer A
        DCD     Timer3B_Handler		        ; Timer 3 subtimer B
        DCD     IntDefaultHandler           ; I2C1 Master and Slave
        DCD     IntDefaultHandler           ; Quadrature Encoder 1
        DCD     IntDefaultHandler           ; CAN0
        DCD     IntDefaultHandler           ; CAN1
        DCD     IntDefaultHandler           ; CAN2
        DCD     Ethernet_Handler           	; Ethernet
        DCD     IntDefaultHandler           ; Hibernate
        DCD     IntDefaultHandler           ; USB0
        DCD     IntDefaultHandler           ; PWM Generator 3
        DCD     IntDefaultHandler           ; uDMA Software Transfer
        DCD     IntDefaultHandler           ; uDMA Error

;******************************************************************************
;
; Useful functions.
;
;******************************************************************************
        EXPORT  DisableInterrupts
        EXPORT  EnableInterrupts
        EXPORT  StartCritical
        EXPORT  EndCritical
        EXPORT  WaitForInterrupt

;*********** DisableInterrupts ***************
; disable interrupts
; inputs:  none
; outputs: none
DisableInterrupts
        CPSID  I
        BX     LR

;*********** EnableInterrupts ***************
; disable interrupts
; inputs:  none
; outputs: none
EnableInterrupts
        CPSIE  I
        BX     LR
		
		EXTERN	StopwatchTimer

;*********** StartCritical ************************
; make a copy of previous I bit, disable interrupts
; inputs:  none
; outputs: previous I bit
StartCritical
        MRS    R0, PRIMASK  ; save old status
        CPSID  I            ; mask all (except faults)
;		PUSH   {LR}
;		BL	   StopwatchTimer; start the stop watch
;		POP	   {LR}
        BX     LR

;*********** EndCritical ************************
; using the copy of previous I bit, restore I bit to previous value
; inputs:  previous I bit
; outputs: none
EndCritical
;		PUSH   {LR}
;		BL	   StopwatchTimer ; Stop the stop watch and save the time since interrupts were disabled
;		POP	   {LR}
        MSR    PRIMASK, R0
        BX     LR

;*********** WaitForInterrupt ************************
; go to low power mode while waiting for the next interrupt
; inputs:  none
; outputs: none
WaitForInterrupt
        WFI
        BX     LR

		EXTERN currentThread
		EXTERN changeThread
spOffset EQU 12
PendSVHandler
SysTickHandler
        CPSID I                 ; mask all (except faults)
		PUSH {LR}               ; save the current LR for the duration of the interrupt
		BL currentThread        ; call currentThread to get the curent TCB's address
		MOVS R12, R0            ; put address of current TCB into r12
		BEQ STExit              ; exit if there are no threads running
		BL changeThread         ; call changeThread to change the thread and return it's TCB address
		MRS R1, MSP             ; save MSP into r1 for the rest of the interrupt
		MRS R2, PSP             ; load PSP into MSP
		MSR MSP, R2
		PUSH {R4-R11}           ; need to save the unsaved registers to current thread's stack
		MRS  R2, MSP            ; update r0 with the latest user stack pointer
		STR  R2, [R12,#spOffset] ; save the current stack pointer to currentTCB
		LDR  R2, [R0,#spOffset]  ; load new stack pointer
		MSR  MSP, R2            ; make MSP match the new PSP
		POP  {R4-R11}           ; pop saved registers
		MRS  R2, MSP            ; update r2 with latest stack pointer
		MSR  PSP, R2
		MSR  MSP, R1            ; restore MSP
STExit  POP {LR}
		CPSIE I
		BX	LR
			
		EXPORT StartFirstThread
StartFirstThread                ; call from thread mode
		CPSID I
		LDR SP, [r0,#spOffset]            ; change the stack pointer to match that of the TCB
		CPSIE I
		POP {PC}
		
		EXPORT EnableUserStack
EnableUserStack
		PUSH {R1}
		MOV R1, #2
		MSR CONTROL, R1	; set ASP to PSP
		MRS R1, MSP		; get MSP
		MSR PSP, R1		; point PSP to the MSP
		POP {R1}
		BX LR

		; EXPORT	OS_Wait
		; EXPORT	OS_Signal
		; EXPORT	OS_InitSemaphore

;;; Wait for semaphore to release
; input: address of semaphore
; output: void
;;;
; OS_Wait
		; LDREX	R1, [R0]
		; SUBS	R1, #1
		; ITT		PL
		; STREXPL R2,R1,[R0]
		; CMPPL	R2,#0
		; BNE		OS_Wait
		; BX		LR
; ;;; Release semaphore
; ; input: address of semaphore
; ; output: void
; ;;;
; OS_Signal
		; LDREX	R1,[R0]
		; ADD		R1,#1
		; STREX	R2,R1,[R0]
		; CMP		R2,#0
		; BNE		OS_Signal
		; BX		LR
; ;;; Initialize semaphore
; ; input: (address of semaphore, initialization value)
; ; output: void
; ;;;
; OS_InitSemaphore
		; STR		R1,[R0] ; Load the initial value into the semaphore
		; BX		LR
;******************************************************************************
;
; This is the code that gets called when the processor first starts execution
; following a reset event.
;
;******************************************************************************
        EXPORT  Reset_Handler
Reset_Handler
        ;
        ; Call the C library enty point that handles startup.  This will copy
        ; the .data section initializers from flash to SRAM and zero fill the
        ; .bss section.
        ;
        IMPORT  __main
        B       __main

;******************************************************************************
;
; This is the code that gets called when the processor receives a NMI.  This
; simply enters an infinite loop, preserving the system state for examination
; by a debugger.
;
;******************************************************************************
NmiSR
        B       NmiSR

;******************************************************************************
;
; This is the code that gets called when the processor receives a fault
; interrupt.  This simply enters an infinite loop, preserving the system state
; for examination by a debugger.
;
;******************************************************************************
		EXTERN 	Debug_DumpThreads
FaultISR
		;;BL		Debug_DumpThreads
        B       FaultISR
EndFault
		B		EndFault

;******************************************************************************
;
; This is the code that gets called when the processor receives an unexpected
; interrupt.  This simply enters an infinite loop, preserving the system state
; for examination by a debugger.
;
;******************************************************************************
IntDefaultHandler
        B       IntDefaultHandler

;******************************************************************************
;
; Make sure the end of this section is aligned.
;
;******************************************************************************
        ALIGN

;******************************************************************************
;
; Some code in the normal code section for initializing the heap and stack.
;
;******************************************************************************
        AREA    |.text|, CODE, READONLY

;******************************************************************************
;
; The function expected of the C library startup code for defining the stack
; and heap memory locations.  For the C library version of the startup code,
; provide this function so that the C library initialization code can find out
; the location of the stack and heap.
;
;******************************************************************************
    IF :DEF: __MICROLIB
        EXPORT  __initial_sp
        EXPORT  __heap_base
        EXPORT __heap_limit
    ELSE
        IMPORT  __use_two_region_memory
        EXPORT  __user_initial_stackheap
__user_initial_stackheap
        LDR     R0, =HeapMem
        LDR     R1, =(StackMem + Stack)
        LDR     R2, =(HeapMem + Heap)
        LDR     R3, =StackMem
        BX      LR
    ENDIF

;******************************************************************************
;
; Make sure the end of this section is aligned.
;
;******************************************************************************
        ALIGN

;******************************************************************************
;
; Tell the assembler that we're done.
;
;******************************************************************************
        END

// this was used as reference https://onlineacademiccommunity.uvic.ca/csc460/

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "os.h"
#include "queue.h"

extern void a_main();

/*===========
  * RTOS Internal
  *===========
  */

/**
  * This internal kernel function is the context switching mechanism.
  * It consists two halves: the top half is called "Exit_Kernel()", and the bottom half is called "Enter_Kernel()".
  * When kernel calls this function, it starts the top half (i.e., exit). Right in
  * the middle, "Cp" is activated; as a result, Cp is running and the kernel is
  * suspended in the middle of this function. When Cp makes a system call,
  * it enters the kernel via the Enter_Kernel() software interrupt into
  * the middle of this function, where the kernel was suspended.
  * After executing the bottom half, the context of Cp is saved and the context
  * of the kernel is restore. Hence, when this function returns, kernel is active
  * again, but Cp is not running any more. 
  * (See file "cswitch.S" for details.)
  */
extern void CSwitch();
extern void Exit_Kernel();    /* this is the same as CSwitch() */

/**
  * Prototypes
  */
void Task_Terminate(void);
static void Dispatch();

/** 
  * Contained in cswitch.S, context switches to the kernel
  */ 
extern void Enter_Kernel();

/**
  * This table contains ALL process descriptors. It doesn't matter what
  * state a task is in.
  */
static PD Process[MAXTHREAD];

/**
  * The number of ticks since the system began
  */
PID time_since_system_start;

/**
  * The process descriptor of the currently RUNNING task.
  */
volatile static PD* Cp; 

/** 
  * Since this is a "full-served" model, the kernel is executing using its own
  * stack. We can allocate a new workspace for this kernel stack, or we can
  * use the stack of the "main()" function, i.e., the initial C runtime stack.
  * (Note: This and the following stack pointers are used primarily by the
  *   context switching code, i.e., CSwitch(), which is written in assembly
  *   language.)
  */         
volatile unsigned char *KernelSp;

/**
  * This is a "shadow" copy of the stack pointer of "Cp", the currently
  * running task. During context switching, we need to save and restore
  * it into the appropriate process descriptor.
  */
volatile unsigned char *CurrentSp;

/** 1 if kernel has been started; 0 otherwise. */
volatile static unsigned int KernelActive;  

/** number of active tasks */
volatile static unsigned int Tasks; 

 /** Keeps track of the process id's. Always goes up. */
volatile static unsigned int pCount;

/** Global tick overflow count */
volatile unsigned int tickOverflowCount = 0;

/** The SystemQueue for tasks */
volatile PD *SysQueue[MAXTHREAD];
volatile int SysCount = 0;

/** The PeriodicQueue for tasks */
volatile PD *PeriodicQueue[MAXTHREAD];
volatile int PeriodicCount = 0;

/** The RRQueue for tasks */
volatile PD *RRQueue[MAXTHREAD];
volatile int RRCount = 0;

/* The array of channel descriptors */
volatile static CD ChannelArray[MAXCHAN];
volatile unsigned int Channels = 0;

/*
 * idle tasks to be called when there is nothing else to run
 * it has lowest priority
 */
static void idle (void)
{
	for(;;)
	{};
}

/**
 * Sets up a task's stack with Task_Terminate() at the bottom,
 * The return address of the function
 * and dummy data to be popped off when the task first runs
 */
PID Kernel_Create_Task_At( volatile PD *p, voidfuncptr f, PRIORITY priority, int arg, int offset, int wcet, int period) {   
	unsigned char *sp;

	sp = (unsigned char *) &(p->workSpace[WORKSPACE-1]);

	//Clear the contents of the workspace
	memset(&(p->workSpace),0,WORKSPACE);

	//Notice that we are placing the address (16-bit) of the functions
	//onto the stack in reverse byte order (least significant first, followed
	//by most significant).  This is because the "return" assembly instructions 
	//(rtn and rti) pop addresses off in BIG ENDIAN (most sig. first, least sig. 
	//second), even though the AT90 is LITTLE ENDIAN machine.

	//Store terminate at the bottom of stack to protect against stack underrun.
	*(unsigned char *)sp-- = ((unsigned int)Task_Terminate) & 0xff;
	*(unsigned char *)sp-- = (((unsigned int)Task_Terminate) >> 8) & 0xff;
	*(unsigned char *)sp-- = 0x00; // make the task forcefully go to Task_Terminate() when the task returns

	//Place return address of function at bottom of stack
	*(unsigned char *)sp-- = ((unsigned int)f) & 0xff;
	*(unsigned char *)sp-- = (((unsigned int)f) >> 8) & 0xff;
	*(unsigned char *)sp-- = 0x00; // Fix 17 bit address problem for PC

	sp = sp - 34;
	
	// set all necessary variables regarding new task
	p->sp = sp;     /* stack pointer into the "workSpace" */
	p->code = f;        /* function to be executed as a task */
	p->request = NONE;
	p->processID = pCount;
	p->priority = priority;
	p->arg = arg;
	p->countdown = -1;
	p->runningTime = 0;
	p->offset = offset;
	p->period = period;
	p->wcet = wcet;

	// increase respective counters
	Tasks++;
	pCount++;

	// set as ready to run
	p->state = READY;

	// enqueue in respective queue according to priority
	if (priority == SYSTEM) {
		enqueue(&p, &SysQueue, &SysCount);
	} else if (priority == PERIODIC) {
		p->countdown = offset;
		enqueuePeriodic(&p, &PeriodicQueue, &PeriodicCount);
	} else if (priority == RR) {
		enqueue(&p, &RRQueue, &RRCount);
	}

	return p->processID;
}

/**
  *  Create a new task
  */
static PID Kernel_Create_Task(voidfuncptr f, PRIORITY priority, int arg, int offset, int wcet, int period) {
	int x;

	if (Tasks == MAXTHREAD) return NULL;  /* Too many task! */

	/* find a DEAD PD that we can use  */
	for (x = 0; x < MAXTHREAD; x++) {
		if (Process[x].state == DEAD) break;
	}

	unsigned int p = Kernel_Create_Task_At( &(Process[x]), f, priority, arg, offset, wcet, period);

	return p;
}

/**
  *  Terminate a task
  */
static void Kernel_Terminate_Task() {
	Cp->state = DEAD;
	Cp->processID = 0;
	Tasks--;
}

/*
 * checks to see if 2 or more periodic tasks are ready to run at the same time
 */
void CheckTimingViolation() {
	int l;
	int counter = 0;
	// count number of periodic tasks ready to run
	for (l = PeriodicCount - 1; l >= 0; l--)  {
		if (PeriodicQueue[l]->countdown == 0) counter++;
	}
	if (counter > 1) OS_Abort(1); // timing violation (2 or more periodic tasks are supposed to run at the same time)
}

/**
  * This internal kernel function is the "scheduler". It chooses the 
  * next task to run, i.e., Cp.
  */
static void Dispatch() {
	CheckTimingViolation(); // check for timing violations
	Cp = dequeue(&SysQueue, &SysCount); // try the sys queue first
	if (PeriodicCount != 0) {
		volatile PD *temp = peek(&PeriodicQueue, &PeriodicCount);
		if (temp->countdown == 0) { // if it is time to run periodic task
			if (Cp == NULL) { // point current process at per task if there is no sys task to run
				Cp = dequeue(&PeriodicQueue, &PeriodicCount);
			} 
		}
	}

	if(Cp == NULL) { // if there are no sys or per tasks to run, run RR
		Cp = dequeue(&RRQueue, &RRCount);
	}

	if (Cp == NULL) { // if there are no tasks to run, run idle
		Cp = &Process[0]; // run idle task
	}

	// point stack at where the new current process needs it
	CurrentSp = Cp->sp;
	// change state of new current process to running
	Cp->state = RUNNING;
}

/**
  * This internal kernel function is the "main" driving loop of this full-served
  * model architecture. Basically, on OS_Start(), the kernel repeatedly
  * requests the next user task's next system call and then invokes the
  * corresponding kernel function on its behalf.
  *
  * This is the main loop of our kernel, called by OS_Start().
  */
static void Next_Kernel_Request() {
	Dispatch();  /* select a new task to run */

	while(1) {
		Cp->request = NONE; /* clear its request */

		/* activate this newly selected task */
		CurrentSp = Cp->sp;

		Exit_Kernel();    /* or CSwitch() */

		/* if this task makes a system call, it will return to here! */

		/* save the Cp's stack pointer */
		Cp->sp = CurrentSp;

		switch(Cp->request){
		case CREATE: // should not be needed
			Cp->response = Kernel_Create_Task( Cp->code, Cp->priority, Cp->arg, -1, -1, -1);
			break;
		case CREATE_PERIODIC: // creates new periodic task
			Cp->response = Kernel_Create_Task( Cp->code, PERIODIC, Cp->arg, Cp->offset, Cp->wcet, Cp->period);
			if(Cp->priority == RR && Cp->offset == 0){
				Cp->request = NEXT;
				goto cnext;
			}
			break;
		case CREATE_RR: // creates new rr task
			Cp->response = Kernel_Create_Task( Cp->code, RR, Cp->arg, -1, -1, -1);
			break;
		case CREATE_SYS: // creates new system task
			Cp->response = Kernel_Create_Task( Cp->code, SYSTEM, Cp->arg, -1, -1, -1);
			if(Cp->priority == SYSTEM) 
				break;
			else
				Cp->request = NEXT;
cnext:	case NEXT:
			Cp->state = READY; // set process to be enqueued as ready to run
			// enqueue in appropriate queue
			if (Cp->priority == SYSTEM) {
				enqueue(&Cp, &SysQueue, &SysCount);
			} else if (Cp->priority == PERIODIC) {
				enqueuePeriodic(&Cp, &PeriodicQueue, &PeriodicCount);
			} else if (Cp->priority == RR) {
				enqueue(&Cp, &RRQueue, &RRCount);
			}
			Dispatch(); // get next task to run
			break;
		case NONE:
			break;
		case CHECK_TIME_VIOLATION:
			CheckTimingViolation(); //check for timing violations
			break;
		case SEND:
			kernel_send(); // send info via CSP
			break;
		case ASYNC_SEND:
			kernel_async_send(); // send info async via CSP
			break;
		case RECEIVE:
			kernel_receive(); // receive info via CSP
			break;
		case TERMINATE:
			/* deallocate all resources used by this task */
			Kernel_Terminate_Task();
			Dispatch(); // get next task to run
			break;

		default:
			/* Houston! we have a problem! */
			break;
		}
	} 
}

/*================
  * RTOS  API  and Stubs
  *================
  */

/**
  * This function initializes the RTOS and must be called first
  */
void OS_Init() {
	int x;

	Tasks = 0;
	KernelActive = 0;
	pCount = 0;

	// set all processes as dead
	for (x = 0; x < MAXTHREAD; x++) {
		memset(&(Process[x]),0,sizeof(PD));
		Process[x].state = DEAD;
		Process[x].processID = 0;
	}

	//set all channels as uninitialized
	for (x = 0; x < MAXCHAN; x++) {
		memset(&(ChannelArray[x]),0,sizeof(CD));
		ChannelArray[x].state = UNITIALIZED;
		ChannelArray[x].channelID = (CHAN) x+1;
	}
}

/**
  * This function starts the RTOS after creating a_main
  */
void OS_Start() {   
	if ( (! KernelActive) && (Tasks > 0)) {
		Disable_Interrupt();

		KernelActive = 1;
		Next_Kernel_Request();
		/* SHOULD NEVER GET HERE!!! */
	}
}

/**
  * Just quits
  */
void OS_Abort(unsigned int error) {
	// set pin to high to indicate error
	PORTC |= (1<<PC7);
	PORTC &= ~(1<<PC7);

	// TODO: add respective codes here
	switch (error) {
		case 1:
			//TIMING VIOLATION
			break;
		case 2:
			//ERROR IN TASK CREATE
			break;
		case 3:
			// WCET > PERIOD
			break;
		case 4:
			// running time exceeded WCET 
			break;
		case 5:
			// periodic task using csp function
			break;
		case 6:
			// csp function error
			break;
		default:
			// unknown error
			break;
	}
	exit(1);
}

/**
  * Application or kernel level task create to setup system call
  */
PID Task_Create(voidfuncptr f, PRIORITY priority, int arg,  int offset,  int wcet,  int period){
	unsigned int p;

	if (KernelActive) {
		Disable_Interrupt();
	
		// set the right request for the kernel
		// based on processes' priority
		if (priority == SYSTEM) {
			Cp->request = CREATE_SYS;
		} else if (priority == PERIODIC) {
			Cp->request = CREATE_PERIODIC;
			Cp->countdown = offset + period;
		} else if (priority == RR) {
			Cp->request = CREATE_RR;
		} else {
			OS_Abort(2);
		}

		// set info to pass to kernel
		Cp->code = f;
		//PRIORITY og_priority = Cp->priority;
		//Cp->priority = priority;
		int og_arg = Cp->arg;
		Cp->arg = arg;
		int og_offset = Cp->offset;
		int og_wcet = Cp->wcet;
		int og_period = Cp->period;
		Cp->offset = offset;
		Cp->wcet = wcet;
		Cp->period = period;
		
		Enter_Kernel();

		// restore the Cp to original values
		p = Cp->response;
		//Cp->priority = og_priority; 
		Cp->offset = og_offset;
		Cp->wcet = og_wcet;
		Cp->period = og_period;
		Cp->arg = og_arg;
	} else { 
	  /* call the RTOS function directly */
	  p = Kernel_Create_Task( f, priority, arg, -1, -1, -1);
	}
	return p;
}

// creates system task
PID Task_Create_System(void (*f)(void), int arg){		
	return Task_Create(f, SYSTEM, arg, -1,-1,-1);		
}		

// creates per task
PID Task_Create_RR(void (*f)(void), int arg){		
	return Task_Create(f, RR, arg, -1,-1,-1);		
}	

// creates rr task	
PID Task_Create_Period(void (*f)(void), int arg, TICK period, TICK wcet, TICK offset){		
	if(wcet >= period) OS_Abort(3);	// as required wcet < period
	return Task_Create(f, PERIODIC, arg, offset, wcet, period);		
}		
/**		
  * Application or kernel level task create to setup system call		
  */		
PID Task_Create_Idle(){		
	unsigned int p;		
	if (KernelActive) {		
		Disable_Interrupt();
		// set appropriate priority and code for idle	
		Cp->code = idle;		
		Cp->priority = IDLE;		
		Cp->arg = 0;		
		Enter_Kernel();		
		p = Cp->response;		
	} else { 		
	  /* call the RTOS function directly */		
	  p = Kernel_Create_Task(idle, IDLE, 0, -1, -1, -1);		
	}		
	return p;		
}

/**
  * Application level task next to setup system call to give up CPU
  *	equivalent to yield
  */
void Task_Next() {
	if (KernelActive) {
		Disable_Interrupt();
		// set the info for periodic so that it runs on time
		if (Cp->priority == PERIODIC) {
			Cp->countdown = Cp->period - Cp->runningTime;
			Cp->runningTime = 0;
		}
		// set kernel request to next
		Cp->request = NEXT;
		Enter_Kernel();
	}
}

/*
 *	Function called by ISR to schedule next task according to current priority
 */
void Run_Next() {
	if (KernelActive) {
		Disable_Interrupt();
		// system task will not be preempted, but check no time violation 
		// was caused by them
		if (Cp->priority == SYSTEM) Cp->request = CHECK_TIME_VIOLATION;
		else if (Cp->priority == PERIODIC) {
			// as requested by reqs, wcet should be less than running time
			if (Cp->runningTime >= Cp->wcet) {
				OS_Abort(4); // exceeded worst case running time
			} else if (SysCount > 0) { // periodic task must be preempted by system task
				Cp->countdown = 0;
				Cp->request = NEXT;
			} else Cp->request = CHECK_TIME_VIOLATION; // check for timing violations
		} else Cp->request = NEXT;
		Enter_Kernel();
	}
}

/**
  * Application level task terminate to setup system call
  */
void Task_Terminate() {
	if (KernelActive) {
		Disable_Interrupt();
		Cp -> request = TERMINATE;
		Enter_Kernel();
		/* never returns here! */
	}
}

/**
  * Application level task getarg to return intiial arg value
  */
int Task_GetArg() {
	return (Cp->arg);
}

 // number of milliseconds since the RTOS boots.
unsigned int Now(){
	volatile uint16_t t = TCNT1;
	volatile unsigned int temp = t;
	volatile float f = ((temp*1.0)/635)*10;
	return time_since_system_start*10 + (unsigned int)f;
}

/**
  * Setup pins and timers
  */
void setup() {
	/** initialize Timer1 16 bit timer */
	Disable_Interrupt();

	/** Timer 1 */
	TCCR1A = 0;                 /** Set TCCR1A register to 0 */
	TCCR1B = 0;                 /** Set TCCR1B register to 0 */

	TCNT1 = 0;                  /** Initialize counter to 0 */

	OCR1A = 624;                /** Compare match register (TOP comparison value) [(16MHz/(100Hz*8)] - 1 */

	TCCR1B |= (1 << WGM12);     /** Turns on CTC mode (TOP is now OCR1A) */

	TCCR1B |= (1 << CS12);      /** Prescaler 256 */

	TIMSK1 |= (1 << OCIE1A);    /** Enable timer compare interrupt */

	/** Timer 3 */
	TCCR3A = 0;                 /** Set TCCR0A register to 0 */
	TCCR3B = 0;                 /** Set TCCR0B register to 0 */

	TCNT3 = 0;                  /** Initialize counter to 0 */

	OCR3A = 62499;              /** Compare match register (TOP comparison value) [(16MHz/(100Hz*8)] - 1 */

	TCCR3B |= (1 << WGM32);     /** Turns on CTC mode (TOP is now OCR1A) */

	TCCR3B |= (1 << CS32);      /** Prescaler 1024 */

	TIMSK3 = (1 << OCIE3A);

	Enable_Interrupt();
}

/**
  * ISR for timer1 (every tick [10 ms]) 
  */
ISR(TIMER1_COMPA_vect) { 
	int i;

	// increase running time of periodic task
	// useful to prevent it running longer than wcet
	// and also for rescheduling on time
	if (Cp->priority == PERIODIC) {
		Cp->runningTime++;
	}

	time_since_system_start++; // used for Now()

	// decrease countdown for all periodic task
	// min val of countdown is 0
	for (i = PeriodicCount-1; i >= 0; i--) 
		if (PeriodicQueue[i]->countdown > 0) 
			PeriodicQueue[i]->countdown -= 1;
	Run_Next();
}

/**
  * ISR for timer3 (every second)
  */


/*
 * A CHAN is a one-way communication channel between at least two tasks. It must be
 * initialized before its use. Chan_Init() returns a CHAN if successful; otherwise
 * it returns NULL.
 */

 CHAN Chan_Init() {
	int x;

	if (Channels == MAXCHAN) return NULL;  /* Too many task! */

	/* find an UNITIALIZED CD that we can use  */
	for (x = 0; x < MAXCHAN; x++) {
		if (ChannelArray[x].state == UNITIALIZED) {
			ChannelArray[x].state = USED;
			ChannelArray[x].numberReceivers = 0;
			break;
		}
	}

	if (x == MAXCHAN) return NULL;
	return ChannelArray[x].channelID;
 }


 /*
  * function for sending synchronously.
  * sender will be blocked if receiver(s) not present
  */
void Send(CHAN ch, int v) {
	if (Cp->priority == PERIODIC) OS_Abort(5); // periodic tasks are not allowed to use csp
	if (ChannelArray[ch - 1].state == UNITIALIZED) OS_Abort(6); // trying to use unitialized channel
	Cp->request = SEND;
	Cp->senderChannel = ch;
	Cp->val = v; // val to pass to receivers
	Enter_Kernel();
}

/*
 * kernel actually sending info between processes using channel
 * also blocks sender if no receivers waiting
 * or it unblocks receivers if they are waiting
 */
void kernel_send() {
	BOOL sys_task_ready = FALSE;
	if (ChannelArray[Cp->senderChannel - 1].numberReceivers == 0) { // no receivers waiting
		if (ChannelArray[Cp->senderChannel - 1].sender == NULL) ChannelArray[Cp->senderChannel - 1].sender = Cp;
		else OS_Abort(6); // cant have more than 1 sender

		Cp->state = BLOCKED; // block sender
		ChannelArray[Cp->senderChannel - 1].val = Cp->val; // save val to pass to receivers
		Dispatch(); // get someone else to run
	} else { //receivers are waiting
		if (ChannelArray[Cp->senderChannel - 1].sender != NULL) OS_Abort(6); // cant have more than 1 sender
		
		int l;
		// for each receiver, set to ready, pass value from sender, and requeue
		for (l = ChannelArray[Cp->senderChannel - 1].numberReceivers - 1; l >= 0; l--)  {
			ChannelArray[Cp->senderChannel - 1].receivers[l]->state = READY;
			ChannelArray[Cp->senderChannel - 1].receivers[l]->val = Cp->val;

			if (ChannelArray[Cp->senderChannel - 1].receivers[l]->priority == SYSTEM) {
				sys_task_ready = TRUE;
				enqueue(&ChannelArray[Cp->senderChannel - 1].receivers[l], &SysQueue, &SysCount);
			} else if (ChannelArray[Cp->senderChannel - 1].receivers[l]->priority == RR) {
				enqueue(&ChannelArray[Cp->senderChannel - 1].receivers[l], &RRQueue, &RRCount);
			}
			ChannelArray[Cp->senderChannel - 1].receivers[l] = NULL;
			ChannelArray[Cp->senderChannel - 1].numberReceivers--;
		}
		ChannelArray[Cp->senderChannel - 1].val = NULL;
		if (sys_task_ready && Cp->priority == RR) {
			enqueue(&Cp, &RRQueue, &RRCount);
			Dispatch();
		}
	}
}

/*
 * function that can be called by processes. calling processes will be blocked
 * if no sender is waiting. also, it unblocks sender if it is waiting
 */
int Recv(CHAN ch) {
	if (Cp->priority == PERIODIC) OS_Abort(5); // periodic tasks are not allowed to use csp
	if (ChannelArray[ch - 1].state == UNITIALIZED) OS_Abort(6); // trying to use unitialized channel
	Cp->request = RECEIVE; // set kernel request to receive
	Cp->receiverChannel = ch; // pass to kernel the selected channel
	Enter_Kernel();
	return Cp->val; //return the received value
}

/*
 * function for kernel to actually receive int from sender
 * if no sender is waiting. also, it unblocks sender if it is waiting
 */
void kernel_receive() {
	BOOL sys_task_ready = FALSE;

	if (ChannelArray[Cp->receiverChannel - 1].sender == NULL) { // no sender waiting
		Cp->state = BLOCKED; // block calling process
		// add current process to receivers array
		enqueue(&Cp, &ChannelArray[Cp->receiverChannel - 1].receivers, &ChannelArray[Cp->receiverChannel - 1].numberReceivers);
		Dispatch(); // get next process to run
	} else { // sender is waiting
		// set sender to ready
		ChannelArray[Cp->receiverChannel - 1].sender->state = READY;
		// grab value to be passed from channel
		Cp->val = ChannelArray[Cp->receiverChannel - 1].val;

		// enqueue sender so it runs again
		if (ChannelArray[Cp->receiverChannel - 1].sender->priority == SYSTEM) {
			sys_task_ready = TRUE;
			enqueue(&ChannelArray[Cp->receiverChannel - 1].sender, &SysQueue, &SysCount);
		} else if (ChannelArray[Cp->receiverChannel - 1].sender->priority == RR) {
			enqueue(&ChannelArray[Cp->receiverChannel - 1].sender, &RRQueue, &RRCount);
		}
		// clear sender and val info
		ChannelArray[Cp->receiverChannel - 1].sender = NULL;
		ChannelArray[Cp->senderChannel - 1].val = NULL;
		if (sys_task_ready && Cp->priority == RR) {
			enqueue(&Cp, &RRQueue, &RRCount);
			Dispatch();
		}
	}
}

/*
 *	pretty much the same as Send() except process is not blocked if no receivers are waiting
 */
void Write(CHAN ch, int v) {
	if (ChannelArray[ch - 1].state == UNITIALIZED) OS_Abort(6); // trying to use unitialized channel

	Cp->request = ASYNC_SEND;
	Cp->senderChannel = ch;
	Cp->val = v;
	Enter_Kernel();
}

/*
 *	pretty much the same as kernel_send() except process is not blocked if no receivers are waiting
 */
void kernel_async_send() {
	BOOL sys_task_ready = FALSE;

	if (ChannelArray[Cp->senderChannel - 1].numberReceivers == 0) { // no receivers waiting
		if (ChannelArray[Cp->senderChannel - 1].sender == NULL) ChannelArray[Cp->senderChannel - 1].sender = Cp;
		else OS_Abort(6); // cant have more than 1 sender
		return; // return without blocking
	} else { //receivers are waiting
		// do the same as kernel_send
		// probably should be abstracted into a separate function to avoid code duplication, but who has time for that
		if (ChannelArray[Cp->senderChannel - 1].sender != NULL) OS_Abort(6); // cant have more than 1 sender
		int l;
		for (l = ChannelArray[Cp->senderChannel - 1].numberReceivers - 1; l >= 0; l--)  {
			ChannelArray[Cp->senderChannel - 1].receivers[l]->state = READY;
			ChannelArray[Cp->senderChannel - 1].receivers[l]->val = Cp->val;

			if (ChannelArray[Cp->senderChannel - 1].receivers[l]->priority == SYSTEM) {
				sys_task_ready = TRUE;
				enqueue(&ChannelArray[Cp->senderChannel - 1].receivers[l], &SysQueue, &SysCount);
			} else if (ChannelArray[Cp->senderChannel - 1].receivers[l]->priority == RR) {
				enqueue(&ChannelArray[Cp->senderChannel - 1].receivers[l], &RRQueue, &RRCount);
			}
			ChannelArray[Cp->senderChannel - 1].receivers[l] = NULL;
			ChannelArray[Cp->senderChannel - 1].numberReceivers--;
		}
		ChannelArray[Cp->senderChannel - 1].val = NULL;
		if (sys_task_ready && Cp->priority == RR) {
			enqueue(&Cp, &RRQueue, &RRCount);
			Dispatch();
		}
	}
}

/**
  * This function boots the OS and creates the first task: a_main
  */
void main() {
	//pin 25
	DDRA |= (1<<PA3);
	PORTA &= ~(1<<PA3);

	//pin 26
	DDRA |= (1<<PA4);
	PORTA &= ~(1<<PA4);

	//pin 27
	DDRA |= (1<<PA5);
	PORTA &= ~(1<<PA5);

	//pin 28
	DDRA |= (1<<PA6);
	PORTA &= ~(1<<PA6);

	//pin 29
	DDRA |= (1<<PA7);
	PORTA &= ~(1<<PA7);

	//pin 30
	DDRC |= (1<<PC7);
	PORTC &= ~(1<<PC7);

	setup();
	OS_Init();
	Task_Create_Idle();
	Task_Create_System(a_main, 42);
	OS_Start();
}


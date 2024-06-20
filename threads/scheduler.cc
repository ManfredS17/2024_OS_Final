// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------


//<TODO>(Wait debug)
// Declare sorting rule of SortedList for L1 & L2 ReadyQueue
// Hint: Funtion Type should be "static int"
static int cmpRemainingBurstTime(Thread* a, Thread* b)
{
    return (a->getRemainingBurstTime() < b->getRemainingBurstTime()) ? 1 : -1;
}

static int cmpID(Thread* a, Thread* b)
{
    return (a->getID() < b->getID()) ? 1 : -1;
}

//<TODO>

Scheduler::Scheduler()
{
//	schedulerType = type;
    // readyList = new List<Thread *>; 
    //<TODO>(Wait debug)
    // Initialize L1, L2, L3 ReadyQueue
	L1ReadyQueue = new SortedList<Thread*>(cmpRemainingBurstTime);
    L2ReadyQueue = new SortedList<Thread*>(cmpID);
    L3ReadyQueue = new List<Thread*>;
    //<TODO>
	toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    //<TODO>(Wait debug)
    // Remove L1, L2, L3 ReadyQueue
    delete L1ReadyQueue, L2ReadyQueue, L3ReadyQueue;
    //<TODO>
    // delete readyList; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    // DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());

    Statistics* stats = kernel->stats;
    //<TODO>(wait debug)
    // According to priority of Thread, put them into corresponding ReadyQueue.
    // After inserting Thread into ReadyQueue, don't forget to reset some values.
    // Hint: L1 ReadyQueue is preemptive SRTN(Shortest Remaining Time Next).
    // When putting a new thread into L1 ReadyQueue, you need to check whether preemption or not.
    int Priority=thread->getPriority();
    if(Priority >=0 && Priority < 50)
    {
        DEBUG('z', "[InsertToQueue] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] is inserted into queue L[3]\n");
        thread->setStatus(READY);
        L3ReadyQueue->Append(thread);
    }
    else if(Priority < 100)
    {
        DEBUG('z',"[InsertToQueue] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] is inserted into queue L[2]\n");
        thread->setStatus(READY);
        L2ReadyQueue->Insert(thread);
    }
    else if(Priority < 150)
    {
        DEBUG('z',"[InsertToQueue] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] is inserted into queue L[1]\n");
        thread->setStatus(READY);
        L1ReadyQueue->Insert(thread);
        //dealing with preemptive SRTN
        if (!L1ReadyQueue->IsEmpty() && L1ReadyQueue->Front()->getRemainingBurstTime() > thread->getRemainingBurstTime())
        {
            kernel->currentThread->Yield();
        }
    }
    //<TODO>
    // readyList->Append(thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    /*if (readyList->IsEmpty()) {
    return NULL;
    } else {
        return readyList->RemoveFront();
    }*/

    //<TODO>(Not finished)
    // a.k.a. Find Next (Thread in ReadyQueue) to Run
    Thread* thread;

    if(!L1ReadyQueue->IsEmpty())
    {
        thread = L1ReadyQueue->RemoveFront();
        DEBUG('z',"[RemoveFromQueue] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] removed from queue L[1]\n");
        return thread;
    }
    else if(!L2ReadyQueue->IsEmpty())
    {
        thread = L2ReadyQueue->RemoveFront();
        DEBUG('z',"[RemoveFromQueue] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] removed from queue L[2]\n");
        return thread;
    }
    else if(!L3ReadyQueue->IsEmpty())
    {
        thread = L3ReadyQueue->RemoveFront();
        DEBUG('z',"[RemoveFromQueue] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] removed from queue L[3]\n");
        return thread;
    }
    else return NULL;
    //<TODO>
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
 
//	cout << "Current Thread" <<oldThread->getName() << "    Next Thread"<<nextThread->getName()<<endl;
   
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	     toBeDestroyed = oldThread;
    }
   
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (oldThread->space != NULL) {	// if this thread is a user program,

        oldThread->SaveUserState(); 	// save the user's CPU registers
	    oldThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    // DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    cout << "Switching from: " << oldThread->getID() << " to: " << nextThread->getID() << endl;
    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << kernel->currentThread->getID());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
#ifdef USER_PROGRAM
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	    oldThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        DEBUG(dbgThread, "toBeDestroyed->getID(): " << toBeDestroyed->getID());
        delete toBeDestroyed;
	    toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    //cout << "Ready list contents:\n";
    L1ReadyQueue->Apply(ThreadPrint);
    L2ReadyQueue->Apply(ThreadPrint);
    L3ReadyQueue->Apply(ThreadPrint);
}

// <TODO> (wait debug)

// Function 1. Function definition of sorting rule of L1 ReadyQueue

// Function 2. Function definition of sorting rule of L2 ReadyQueue

// Function 3. Scheduler::UpdatePriority()
// Hint:
// 1. ListIterator can help.
// 2. Update WaitTime and priority in Aging situations
// 3. After aging, Thread may insert to different ReadyQueue

void 
Scheduler::UpdatePriority()
{
    ListIterator<Thread*>* iterL3 = new ListIterator<Thread*>(L3ReadyQueue);
    //For all jobs in L1
    while (!iterL3->IsDone()) 
    {
        Thread* thread = iterL3->Item();
        thread->setWaitTime(thread->getWaitTime()+100);
        if (thread->getWaitTime() > 400) 
        {
            int Priority=thread->getPriority();
            thread->setPriority(Priority+10);
            DEBUG('z',"[UpdatePriority] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] changes its priority from ["<<Priority<<"] to ["<<Priority+10<<"]\n");
            thread->setWaitTime(0);
        }
        if (thread->getPriority() >49) {
            L3ReadyQueue->Remove(thread);
            DEBUG('z',"[RemoveFromQueue] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] removed from queue L[3]\n");
            L2ReadyQueue->Insert(thread);
            DEBUG('z',"[RemoveFromQueue] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] is insert into queue L[2]\n");
        }
        iterL3->Next();
    }
    delete iterL3;

   
    ListIterator<Thread*>* iterL2 = new ListIterator<Thread*>(L2ReadyQueue);
    while (!iterL2->IsDone()) 
    {
        Thread* thread = iterL2->Item();
        thread->setWaitTime(thread->getWaitTime()+100);
        if (thread->getWaitTime() > 400) 
        {
            int Priority=thread->getPriority();
            thread->setPriority(Priority+10);
            DEBUG('z',"[UpdatePriority] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] changes its priority from ["<<Priority<<"] to ["<<Priority+10<<"]\n");
            thread->setWaitTime(0);
        }
        if (thread->getPriority() >99) 
        {
            L2ReadyQueue->Remove(thread);
            DEBUG('z',"[RemoveFromQueue] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] removed from queue L[2]\n");
            L1ReadyQueue->Insert(thread);
            DEBUG('z',"[RemoveFromQueue] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] is insert into queue L[1]\n");
        }
        iterL2->Next();
    }
    delete iterL2;


    ListIterator<Thread*>* iterL1 = new ListIterator<Thread*>(L1ReadyQueue);
    while (!iterL1->IsDone()) 
    {
        Thread* thread = iterL1->Item();
        thread->setWaitTime(thread->getWaitTime()+100);
        if (thread->getWaitTime() > 400) 
        {
            int Priority=thread->getPriority();
            thread->setPriority(Priority+10);
            DEBUG('z',"[UpdatePriority] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] changes its priority from ["<<Priority<<"] to ["<<Priority+10<<"]\n");
        }
        iterL1->Next();
    }
    delete iterL1;
}

// <TODO>

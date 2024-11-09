/*
 File: scheduler.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  head = nullptr;
  tail = nullptr;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  // get the next thread in ready queue
  Thread * callthread = head;
  // remove the current one from queue
  head = head->getnextthread();
  // check if next is nullptr
  if (head == nullptr) {
    tail = nullptr;
  }
  // dispatch for context switch
  Thread::dispatch_to(callthread);
}

void Scheduler::resume(Thread * _thread) {
  // Check if anything in ready queue
  // if nothing place at head of ready queue
  if (head == nullptr)
  {
    head = _thread;
    head->setnextthread(nullptr);
    tail = _thread;
  }
  // If others in queue append to end of queue
  // then set as new end of queue
  else
  {
    tail->setnextthread(_thread);
    tail = _thread;
    tail->setnextthread(nullptr);
  }
}

void Scheduler::add(Thread * _thread) {
  // call resume as it has same functionality
  resume(_thread);
}

void Scheduler::terminate(Thread * _thread) {
  // set variable to traverse ready queue
  Thread * check_thread = head;
  // case where thread to be removed is head
  if (_thread == check_thread)
  { 
    // if tail is equal to head set both to nullptr
    if (tail == head)
    {
      head = nullptr;
      tail = nullptr;
      return;
    }
    // otherwise just set head to next in queue
    else
    {
      head = head->getnextthread();
      return;
    }
  }
  // control for loop
  bool inqueue = true;
  // loop thru ready queue till we find thread to remove
  while (inqueue && (check_thread->getnextthread() != nullptr))
  {
    // found thread to remove
    if (_thread == check_thread->getnextthread())
    {
      //remove thread
      Thread * swap = check_thread->getnextthread();
      check_thread->setnextthread(swap->getnextthread());
      inqueue = false;
    }
    // get next thread
    check_thread = check_thread->getnextthread();
  }
}
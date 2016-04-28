// Abstraction of select call into "event-handling" to make programming
// easier.
//
// This code is based in large part on rp-l2tpd, and
// carries forth its copyrights and licensing.
//
// Copyright (C) 2002 by Roaring Penguin Software Inc.
//
// This software may be distributed under the terms of the GNU General
// Public License, Version 2, or (at your option) any later version.
//
// LIC: GPL

#include "includes.h"

// Kludge for figuring out NSIG 
#ifdef NSIG
#define MAX_SIGNALS NSIG
#elif defined(_NSIG)
#define MAX_SIGNALS _NSIG
#else
#define MAX_SIGNALS 256			// Should be safe... 
#endif

#define EVENT_FLAG_DELETED 256

#define EVENT_DEBUG(x) ((void) 0)

// Handler structure
struct EventHandler
{
	EventHandler *next;			// Link in list
	int fd;						// File descriptor for select
	unsigned int flags;			// Select on read or write; enable timeout
	struct timeval tmout;		// Absolute time for timeout
	EventCallbackFunc fn;		// Callback function
	void *data;					// Extra data to pass to callback
};

// Selector structure
struct EventSelector
{
	EventHandler *handlers;		// Linked list of EventHandlers
	int nestLevel;				// Event-handling nesting level
	int opsPending;				// True if operations are pending
	int destroyPending;			// If true, a destroy is pending
};

// A structure for a "synchronous" signal handler 
struct SynchronousSignalHandler
{
	int fired;					// Have we received this signal? 
	void (*handler)(int sig);	// Handler function				 
};

// A structure for calling back when a child dies 
struct ChildEntry
{
	hash_bucket hash;
	void (*handler)(pid_t pid, int status, void *data);
	pid_t pid;
	void *data;
};

static struct SynchronousSignalHandler SignalHandlers[MAX_SIGNALS];
static int Pipe[2] = {-1, -1};
static EventHandler *PipeHandler = NULL;
static sig_atomic_t PipeFull = 0;
static hash_table child_process_table;


/**********************************************************************
* %FUNCTION: DestroyHandler
* %ARGUMENTS:
*  eh -- an event handler
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Destroys handler
***********************************************************************/
static void DestroyHandler(EventHandler *eh)
{
	EVENT_DEBUG(("DestroyHandler(eh=%p)\n", eh));
	free(eh);
}

/**********************************************************************
* %FUNCTION: DestroySelector
* %ARGUMENTS:
*  es -- an event selector
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Destroys selector and all associated handles.
***********************************************************************/
static void DestroySelector(EventSelector *es)
{
	EventHandler *cur, *next;

	for (cur=es->handlers; cur; cur=next)
	{
		next = cur->next;
		DestroyHandler(cur);
	}
	free(es);
}

/**********************************************************************
* %FUNCTION: DoPendingChanges
* %ARGUMENTS:
*  es -- an event selector
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Makes all pending insertions and deletions happen.
***********************************************************************/
static void DoPendingChanges(EventSelector *es)
{
	EventHandler *cur, *prev, *next;

	es->opsPending = 0;

	// If selector is to be deleted, do it and skip everything else
	if (es->destroyPending) {
		DestroySelector(es);
		return;
	}

	// Do deletions
	cur = es->handlers;
	prev = NULL;
	while(cur) {
		if (!(cur->flags & EVENT_FLAG_DELETED)) {
			prev = cur;
			cur = cur->next;
			continue;
		}

		// Unlink from list
		if (prev) {
			prev->next = cur->next;
		} else {
			es->handlers = cur->next;
		}
		next = cur->next;
		DestroyHandler(cur);
		cur = next;
	}
}

/**********************************************************************
* %FUNCTION: Event_DestroySelector
* %ARGUMENTS:
*  es -- EventSelector to destroy
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Destroys an EventSelector.  Destruction may be delayed if we
*  are in the HandleEvent function.
***********************************************************************/
void Event_DestroySelector(EventSelector *es)
{
	if (es->nestLevel)
	{
		es->destroyPending = 1;
		es->opsPending = 1;
		return;
	}
	DestroySelector(es);
}

/**********************************************************************
* %FUNCTION: Event_CreateSelector
* %ARGUMENTS:
*  None
* %RETURNS:
*  A newly-allocated EventSelector, or NULL if out of memory.
* %DESCRIPTION:
*  Creates a new EventSelector.
***********************************************************************/
EventSelector *Event_CreateSelector(void)
{
	EventSelector *es = (EventSelector *)malloc(sizeof(EventSelector));
	if (!es) return NULL;
	es->handlers = NULL;
	es->nestLevel = 0;
	es->destroyPending = 0;
	es->opsPending = 0;
	EVENT_DEBUG(("CreateSelector() -> %p\n", (void *) es));
	return es;
}

/**********************************************************************
* %FUNCTION: Event_HandleEvent
* %ARGUMENTS:
*  es -- EventSelector
* %RETURNS:
*  0 if OK, non-zero on error.	errno is set appropriately.
* %DESCRIPTION:
*  Handles a single event (uses select() to wait for an event.)
***********************************************************************/
int Event_HandleEvent(EventSelector *es)
{
	fd_set readfds, writefds;
	fd_set *rd, *wr;
	unsigned int flags;

	struct timeval abs_timeout, now;
	struct timeval timeout;
	struct timeval *tm;
	EventHandler *eh;

	int r = 0;
	int errno_save = 0;
	int foundTimeoutEvent = 0;
	int foundReadEvent = 0;
	int foundWriteEvent = 0;
	int maxfd = -1;
	int pastDue;

	EVENT_DEBUG(("Enter Event_HandleEvent(es=%p)\n", (void *) es));

	// Build the select sets
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	eh = es->handlers;
	for (eh=es->handlers; eh; eh=eh->next) {
		if (eh->flags & EVENT_FLAG_DELETED) continue;
		if (eh->flags & EVENT_FLAG_READABLE) {
			foundReadEvent = 1;
			FD_SET(eh->fd, &readfds);
			if (eh->fd > maxfd) maxfd = eh->fd;
		}
		if (eh->flags & EVENT_FLAG_WRITEABLE) {
			foundWriteEvent = 1;
			FD_SET(eh->fd, &writefds);
			if (eh->fd > maxfd) maxfd = eh->fd;
		}
		if (eh->flags & EVENT_TIMER_BITS) {
			if (!foundTimeoutEvent) {
				abs_timeout = eh->tmout;
				foundTimeoutEvent = 1;
			} else {
				if (eh->tmout.tv_sec < abs_timeout.tv_sec ||
					(eh->tmout.tv_sec == abs_timeout.tv_sec &&
					 eh->tmout.tv_usec < abs_timeout.tv_usec)) {
					abs_timeout = eh->tmout;
				}
			}
		}
	}
	if (foundReadEvent) {
		rd = &readfds;
	} else {
		rd = NULL;
	}
	if (foundWriteEvent) {
		wr = &writefds;
	} else {
		wr = NULL;
	}

	if (foundTimeoutEvent) {
		gettimeofday(&now, NULL);
		// Convert absolute timeout to relative timeout for select
		timeout.tv_usec = abs_timeout.tv_usec - now.tv_usec;
		timeout.tv_sec = abs_timeout.tv_sec - now.tv_sec;
		if (timeout.tv_usec < 0) {
			timeout.tv_usec += 1000000;
			timeout.tv_sec--;
		}
		if (timeout.tv_sec < 0 ||
			(timeout.tv_sec == 0 && timeout.tv_usec < 0)) {
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;
		}
		tm = &timeout;
	} else {
		tm = NULL;
	}

	if (foundReadEvent || foundWriteEvent || foundTimeoutEvent) {
		for(;;) {
			r = select(maxfd+1, rd, wr, NULL, tm);
			if (r < 0) {
				if (errno == EINTR) continue;
			}
			break;
		}
	}

	if (foundTimeoutEvent) gettimeofday(&now, NULL);
	errno_save = errno;
	es->nestLevel++;

	if (r >= 0) {
		// Call handlers
		for (eh=es->handlers; eh; eh=eh->next) {

			// Pending delete for this handler?  Ignore it
			if (eh->flags & EVENT_FLAG_DELETED) continue;

			flags = 0;
			if ((eh->flags & EVENT_FLAG_READABLE) &&
				FD_ISSET(eh->fd, &readfds)) {
				flags |= EVENT_FLAG_READABLE;
			}
			if ((eh->flags & EVENT_FLAG_WRITEABLE) &&
				FD_ISSET(eh->fd, &writefds)) {
				flags |= EVENT_FLAG_WRITEABLE;
			}
			if (eh->flags & EVENT_TIMER_BITS) {
				pastDue = (eh->tmout.tv_sec < now.tv_sec ||
						   (eh->tmout.tv_sec == now.tv_sec &&
							eh->tmout.tv_usec <= now.tv_usec));
				if (pastDue) {
					flags |= EVENT_TIMER_BITS;
					if (eh->flags & EVENT_FLAG_TIMER) {
						// Timer events are only called once
						es->opsPending = 1;
						eh->flags |= EVENT_FLAG_DELETED;
					}
				}
			}
			// Do callback
			if (flags) {
				EVENT_DEBUG(("Enter callback: eh=%p flags=%u\n", eh, flags));
				eh->fn(es, eh->fd, flags, eh->data);
				EVENT_DEBUG(("Leave callback: eh=%p flags=%u\n", eh, flags));
			}
		}
	}

	es->nestLevel--;

	if (!es->nestLevel && es->opsPending) {
		DoPendingChanges(es);
	}
	errno = errno_save;
	return r;
}

/**********************************************************************
* %FUNCTION: Event_DelHandler
* %ARGUMENTS:
*  es -- event selector
*  eh -- event handler
* %RETURNS:
*  0 if OK, non-zero if there is an error
* %DESCRIPTION:
*  Deletes the event handler eh
***********************************************************************/
int Event_DelHandler(EventSelector *es, EventHandler *eh)
{
	// Scan the handlers list
	EventHandler *cur, *prev;
	EVENT_DEBUG(("Event_DelHandler(es=%p, eh=%p)\n", es, eh));
	for (cur=es->handlers, prev=NULL; cur; prev=cur, cur=cur->next) {
		if (cur == eh) {
			if (es->nestLevel) {
				eh->flags |= EVENT_FLAG_DELETED;
				es->opsPending = 1;
				return 0;
			} else {
				if (prev) prev->next = cur->next;
				else	  es->handlers = cur->next;

				DestroyHandler(cur);
				return 0;
			}
		}
	}

	// Handler not found
	return 1;
}

/**********************************************************************
* %FUNCTION: Event_AddHandler
* %ARGUMENTS:
*  es -- event selector
*  fd -- file descriptor to watch
*  flags -- combination of EVENT_FLAG_READABLE and EVENT_FLAG_WRITEABLE
*  fn -- callback function to call when event is triggered
*  data -- extra data to pass to callback function
* %RETURNS:
*  A newly-allocated EventHandler, or NULL.
***********************************************************************/
EventHandler *Event_AddHandler(EventSelector *es, int fd, unsigned int flags, EventCallbackFunc fn, void *data)
{
	EventHandler *eh;

	// Specifically disable timer and deleted flags
	flags &= (~(EVENT_TIMER_BITS | EVENT_FLAG_DELETED));

	// Bad file descriptor
	if (fd < 0) {
		errno = EBADF;
		return NULL;
	}

	eh = (EventHandler *)malloc(sizeof(EventHandler));
	if (!eh) return NULL;
	eh->fd = fd;
	eh->flags = flags;
	eh->tmout.tv_usec = 0;
	eh->tmout.tv_sec = 0;
	eh->fn = fn;
	eh->data = data;

	// Add immediately.  This is safe even if we are in a handler.
	eh->next = es->handlers;
	es->handlers = eh;

	EVENT_DEBUG(("Event_AddHandler(es=%p, fd=%d, flags=%u) -> %p\n", es, fd, flags, eh));
	return eh;
}

/**********************************************************************
* %FUNCTION: Event_AddHandlerWithTimeout
* %ARGUMENTS:
*  es -- event selector
*  fd -- file descriptor to watch
*  flags -- combination of EVENT_FLAG_READABLE and EVENT_FLAG_WRITEABLE
*  t -- Timeout after which to call handler, even if not readable/writable.
*		If t.tv_sec < 0, calls normal Event_AddHandler with no timeout.
*  fn -- callback function to call when event is triggered
*  data -- extra data to pass to callback function
* %RETURNS:
*  A newly-allocated EventHandler, or NULL.
***********************************************************************/
EventHandler *Event_AddHandlerWithTimeout(EventSelector *es, int fd, unsigned int flags, struct timeval t, EventCallbackFunc fn, void *data)
{
	EventHandler *eh;
	struct timeval now;

	// If timeout is negative, just do normal non-timing-out event
	if (t.tv_sec < 0 || t.tv_usec < 0) {
		return Event_AddHandler(es, fd, flags, fn, data);
	}

	// Specifically disable timer and deleted flags
	flags &= (~(EVENT_FLAG_TIMER | EVENT_FLAG_DELETED));
	flags |= EVENT_FLAG_TIMEOUT;

	// Bad file descriptor?
	if (fd < 0) {
		errno = EBADF;
		return NULL;
	}

	// Bad timeout?
	if (t.tv_usec >= 1000000) {
		errno = EINVAL;
		return NULL;
	}

	eh = (EventHandler *)malloc(sizeof(EventHandler));
	if (!eh) return NULL;

	// Convert time interval to absolute time
	gettimeofday(&now, NULL);

	t.tv_sec += now.tv_sec;
	t.tv_usec += now.tv_usec;
	if (t.tv_usec >= 1000000) {
		t.tv_usec -= 1000000;
		t.tv_sec++;
	}

	eh->fd = fd;
	eh->flags = flags;
	eh->tmout = t;
	eh->fn = fn;
	eh->data = data;

	// Add immediately.  This is safe even if we are in a handler.
	eh->next = es->handlers;
	es->handlers = eh;

	EVENT_DEBUG(("Event_AddHandlerWithTimeout(es=%p, fd=%d, flags=%u, t=%d/%d) -> %p\n", es, fd, flags, t.tv_sec, t.tv_usec, eh));
	return eh;
}


/**********************************************************************
* %FUNCTION: Event_AddTimerHandler
* %ARGUMENTS:
*  es -- event selector
*  t -- time interval after which to trigger event
*  fn -- callback function to call when event is triggered
*  data -- extra data to pass to callback function
* %RETURNS:
*  A newly-allocated EventHandler, or NULL.
***********************************************************************/
EventHandler *Event_AddTimerHandler(EventSelector *es, struct timeval t, EventCallbackFunc fn, void *data)
{
	EventHandler *eh;
	struct timeval now;

	// Check time interval for validity
	if (t.tv_sec < 0 || t.tv_usec < 0 || t.tv_usec >= 1000000) {
		errno = EINVAL;
		return NULL;
	}

	eh = (EventHandler *)malloc(sizeof(EventHandler));
	if (!eh) return NULL;

	// Convert time interval to absolute time
	gettimeofday(&now, NULL);

	t.tv_sec += now.tv_sec;
	t.tv_usec += now.tv_usec;
	if (t.tv_usec >= 1000000) {
		t.tv_usec -= 1000000;
		t.tv_sec++;
	}

	eh->fd = -1;
	eh->flags = EVENT_FLAG_TIMER;
	eh->tmout = t;
	eh->fn = fn;
	eh->data = data;

	// Add immediately.  This is safe even if we are in a handler.
	eh->next = es->handlers;
	es->handlers = eh;

	EVENT_DEBUG(("Event_AddTimerHandler(es=%p, t=%d/%d) -> %p\n", es, t.tv_sec,t.tv_usec, eh));
	return eh;
}

/**********************************************************************
* %FUNCTION: Event_GetCallback
* %ARGUMENTS:
*  eh -- the event handler
* %RETURNS:
*  The callback function
***********************************************************************/
EventCallbackFunc Event_GetCallback(EventHandler *eh)
{
	return eh->fn;
}

/**********************************************************************
* %FUNCTION: Event_GetData
* %ARGUMENTS:
*  eh -- the event handler
* %RETURNS:
*  The "data" field.
***********************************************************************/
void *Event_GetData(EventHandler *eh)
{
	return eh->data;
}

/**********************************************************************
* %FUNCTION: Event_SetCallbackAndData
* %ARGUMENTS:
*  eh -- the event handler
*  fn -- new callback function
*  data -- new data value
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Sets the callback function and data fields.
***********************************************************************/
void Event_SetCallbackAndData(EventHandler *eh, EventCallbackFunc fn, void *data)
{
	eh->fn = fn;
	eh->data = data;
}

/**********************************************************************
* %FUNCTION: Event_ChangeTimeout
* %ARGUMENTS:
*  h -- event handler
*  t -- new timeout
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Changes timeout of event handler to be "t" seconds in the future.
***********************************************************************/
void Event_ChangeTimeout(EventHandler *h, struct timeval t)
{
	struct timeval now;

	// Check time interval for validity
	if (t.tv_sec < 0 || t.tv_usec < 0 || t.tv_usec >= 1000000) {
		return;
	}
	// Convert time interval to absolute time
	gettimeofday(&now, NULL);

	t.tv_sec += now.tv_sec;
	t.tv_usec += now.tv_usec;
	if (t.tv_usec >= 1000000) {
		t.tv_usec -= 1000000;
		t.tv_sec++;
	}

	h->tmout = t;
}


static unsigned int child_hash(void *data)
{
	return (unsigned int) ((struct ChildEntry *) data)->pid;
}

static int child_compare(void *d1, void *d2)
{
	return ((struct ChildEntry *)d1)->pid != ((struct ChildEntry *)d2)->pid;
}

/**********************************************************************
* %FUNCTION: DoPipe
* %ARGUMENTS:
*  es -- event selector
*  fd -- readable file descriptor
*  flags -- flags from event system
*  data -- ignored
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Called when an async signal handler wants attention.  This function
*  fires all "synchronous" signal handlers.
***********************************************************************/
static void DoPipe(EventSelector *es, int fd, unsigned int flags, void *data)
{
	char buf[64];
	int i;

	// Clear buffer 
	read(fd, buf, 64);
	PipeFull = 0;

	// Fire handlers 
	for (i=0; i<MAX_SIGNALS; i++)
	{
		if (SignalHandlers[i].fired && SignalHandlers[i].handler)
		{
			SignalHandlers[i].handler(i);
		}
		SignalHandlers[i].fired = 0;
	}
}

/**********************************************************************
* %FUNCTION: sig_handler
* %ARGUMENTS:
*  sig -- signal number
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Marks a signal as having "fired"; fills IPC pipe.
***********************************************************************/
static void sig_handler(int sig)
{
	if (sig <0 || sig > MAX_SIGNALS)
	{
		// Ooops... 
		return;
	}
	SignalHandlers[sig].fired = 1;
	if (!PipeFull)
	{
		write(Pipe[1], &sig, 1);
		PipeFull = 1;
	}
}

/**********************************************************************
* %FUNCTION: child_handler
* %ARGUMENTS:
*  sig -- signal number (whoop-dee-doo)
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Called *SYNCHRONOUSLY* to reap dead children.
***********************************************************************/
static void child_handler(int sig)
{
	int status;
	int pid;
	struct ChildEntry *ce;
	struct ChildEntry candidate;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		candidate.pid = (pid_t) pid;
		ce = (ChildEntry *)hash_find(&child_process_table, &candidate);
		if (ce)
		{
			if (ce->handler)
			{
				ce->handler(pid, status, ce->data);
			}
			hash_remove(&child_process_table, ce);
			free(ce);
		}
	}
}

/**********************************************************************
* %FUNCTION: SetupPipes (static)
* %ARGUMENTS:
*  es -- event selector
* %RETURNS:
*  0 on success; -1 on failure
* %DESCRIPTION:
*  Sets up pipes with an event handler to handle IPC from a signal handler
***********************************************************************/
static int SetupPipes(EventSelector *es)
{
	// If already done, do nothing 
	if (PipeHandler)
	{
		return 0;
	}

	// Initialize the child-process hash table 
	hash_init(&child_process_table, offsetof(struct ChildEntry, hash), child_hash, child_compare);

	// Open pipe to self 
	if (pipe(Pipe) < 0)
	{
		return -1;
	}

	PipeHandler = Event_AddHandler(es, Pipe[0], EVENT_FLAG_READABLE, DoPipe, NULL);

	if (!PipeHandler)
	{
		int old_errno = errno;
		close(Pipe[0]);
		close(Pipe[1]);
		errno = old_errno;
		return -1;
	}
	return 0;
}

/**********************************************************************
* %FUNCTION: Event_HandleSignal
* %ARGUMENTS:
*  es -- event selector
*  sig -- signal number
*  handler -- handler to call when signal is raised.  Handler is called
*			  "synchronously" as events are processed by event loop.
* %RETURNS:
*  0 on success, -1 on error.
* %DESCRIPTION:
*  Sets up a "synchronous" signal handler.
***********************************************************************/
int Event_HandleSignal(EventSelector *es, int sig, void (*handler)(int sig))
{
	struct sigaction act;

	if (SetupPipes(es) < 0)
	{
		return -1;
	}

	act.sa_handler = sig_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
#ifdef SA_RESTART
	act.sa_flags |= SA_RESTART;
#endif
	if (sig == SIGCHLD)
	{
		act.sa_flags |= SA_NOCLDSTOP;
	}
	if (sigaction(sig, &act, NULL) < 0)
	{
		return -1;
	}

	SignalHandlers[sig].handler = handler;

	return 0;
}

/**********************************************************************
* %FUNCTION: Event_HandleChildExit
* %ARGUMENTS:
*  es -- event selector
*  pid -- process-ID of child to wait for
*  handler -- function to call when child exits
*  data -- data to pass to handler when child exits
* %RETURNS:
*  0 on success, -1 on failure.
* %DESCRIPTION:
*  Sets things up so that when a child exits, handler() will be called
*  with the pid of the child and "data" as arguments.  The call will
*  be synchronous (part of the normal event loop on es).
***********************************************************************/
int Event_HandleChildExit(EventSelector *es, pid_t pid, void (*handler)(pid_t, int, void *), void *data)
{
	struct ChildEntry *ce;

	if (Event_HandleSignal(es, SIGCHLD, child_handler) < 0)
	{
		return -1;
	}
	ce = (ChildEntry *)malloc(sizeof(struct ChildEntry));
	if (!ce) return -1;
	ce->pid = pid;
	ce->data = data;
	ce->handler = handler;
	hash_insert(&child_process_table, ce);
	return 0;
}

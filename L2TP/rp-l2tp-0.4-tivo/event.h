// Flags
#define EVENT_FLAG_READABLE 1
#define EVENT_FLAG_WRITEABLE 2
#define EVENT_FLAG_WRITABLE EVENT_FLAG_WRITEABLE

// This is strictly a timer event
#define EVENT_FLAG_TIMER 4

// This is a read or write event with an associated timeout
#define EVENT_FLAG_TIMEOUT 8

#define EVENT_TIMER_BITS (EVENT_FLAG_TIMER | EVENT_FLAG_TIMEOUT)

struct EventSelector;
struct EventHandler;

typedef void (*EventCallbackFunc)(struct EventSelector *es, int fd, unsigned int flags, void *data);


void Event_DestroySelector(EventSelector *es);
EventSelector *Event_CreateSelector(void);
int Event_HandleEvent(EventSelector *es);
int Event_DelHandler(EventSelector *es, EventHandler *eh);
EventHandler *Event_AddHandler(EventSelector *es, int fd, unsigned int flags, EventCallbackFunc fn, void *data);
EventHandler *Event_AddHandlerWithTimeout(EventSelector *es, int fd, unsigned int flags, struct timeval t, EventCallbackFunc fn, void *data);
EventHandler *Event_AddTimerHandler(EventSelector *es, struct timeval t, EventCallbackFunc fn, void *data);
EventCallbackFunc Event_GetCallback(EventHandler *eh);
void *Event_GetData(EventHandler *eh);
void Event_SetCallbackAndData(EventHandler *eh, EventCallbackFunc fn, void *data);
void Event_ChangeTimeout(EventHandler *h, struct timeval t);
int Event_HandleSignal(EventSelector *es, int sig, void (*handler)(int sig));
int Event_HandleChildExit(EventSelector *es, pid_t pid, void (*handler)(pid_t, int, void *), void *data);


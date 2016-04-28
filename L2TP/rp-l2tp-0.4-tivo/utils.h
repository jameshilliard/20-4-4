#define L2TP_RANDOM_FILL(x) l2tp_random_fill(&(x), sizeof(x))

typedef void (*l2tp_shutdown_func)(void *);

int l2tp_register_shutdown_handler(l2tp_shutdown_func f, void *data);
void l2tp_cleanup(void);
char const * l2tp_get_errmsg(void);
void l2tp_set_errmsg(char const *fmt, ...);
void l2tp_random_init(void);
void l2tp_random_fill(void *ptr, size_t size);

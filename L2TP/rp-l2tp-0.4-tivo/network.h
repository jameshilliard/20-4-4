extern int Sock;
extern char Hostname[MAX_HOSTNAME];

void l2tp_network_uninit(EventSelector *es);
int l2tp_network_init(EventSelector *es,const char *interfaceName);

struct l2tp_dgram;

int l2tp_dgram_validate_avp(uint16_t vendor, uint16_t type, uint16_t len, int mandatory);
l2tp_dgram *l2tp_dgram_new(size_t len);
l2tp_dgram *l2tp_dgram_new_control(uint16_t msg_type, uint16_t tid, uint16_t sid);
void l2tp_dgram_free(l2tp_dgram *dgram);
l2tp_dgram *l2tp_dgram_take_from_wire(struct sockaddr_in *from);
int l2tp_dgram_send_to_wire(l2tp_dgram const *dgram, struct sockaddr_in const *to);
int l2tp_dgram_add_avp(l2tp_dgram *dgram, l2tp_tunnel *tunnel, int mandatory, uint16_t len, uint16_t vendor, uint16_t type,const void *val);
unsigned char *l2tp_dgram_pull_avp(l2tp_dgram *dgram, l2tp_tunnel *tunnel, int *mandatory, int *hidden, uint16_t *len, uint16_t *vendor, uint16_t *type, int *err);
unsigned char *l2tp_dgram_search_avp(l2tp_dgram *dgram, l2tp_tunnel *tunnel, int *mandatory, int *hidden, uint16_t *len, uint16_t vendor, uint16_t type);
int l2tp_dgram_send_ppp_frame(l2tp_session *ses, unsigned char *buf, int len);

char const *l2tp_debug_avp_type_to_str(uint16_t type);
char const *l2tp_debug_message_type_to_str(uint16_t type);
char const *l2tp_debug_tunnel_to_str(l2tp_tunnel *tunnel);
char const *l2tp_debug_session_to_str(l2tp_session *session);
void l2tp_db(int what, char const *fmt, ...);
void l2tp_debug_set_bitmask(unsigned long mask);
char const * l2tp_debug_describe_dgram(l2tp_dgram const *dgram);

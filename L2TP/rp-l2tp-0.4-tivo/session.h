void l2tp_session_hash_init(hash_table *tab);
void l2tp_session_free(l2tp_session *ses, char const *reason);
l2tp_session *l2tp_session_call_lns(l2tp_peer *peer, char const *calling_number, EventSelector *es, void *privateData);
void l2tp_session_notify_tunnel_open(l2tp_session *ses);
void l2tp_session_send_CDN(l2tp_session *ses, int result_code, int error_code, char const *fmt, ...);
void l2tp_session_handle_CDN(l2tp_session *ses, l2tp_dgram *dgram);
void l2tp_session_handle_ICRP(l2tp_session *ses, l2tp_dgram *dgram);
void l2tp_session_handle_ICCN(l2tp_session *ses, l2tp_dgram *dgram);
char const *l2tp_session_state_name(l2tp_session *ses);

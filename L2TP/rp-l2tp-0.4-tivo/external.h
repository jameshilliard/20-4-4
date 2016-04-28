void handle_ppp_frame(l2tp_session *ses, unsigned char *buf, size_t len);
void close_ppp_session(l2tp_session *ses, char const *reason);
int establish_ppp_session(l2tp_session *ses);
void set_ppp_session_command(const char *command);

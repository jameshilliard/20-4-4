// Code for managing L2TP sessions
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

static uint32_t call_serial_number = 0;

static char *state_names[] =
{
	"idle",
	"wait-tunnel",
	"wait-reply",
	"wait-connect",
	"established"
};

/**********************************************************************
* %FUNCTION: session_compute_hash
* %ARGUMENTS:
*  data -- a session
* %RETURNS:
*  The session ID
* %DESCRIPTION:
*  Returns a hash value for a session
***********************************************************************/
static unsigned int session_compute_hash(void *data)
{
	return (unsigned int) ((l2tp_session *) data)->my_id;
}

/**********************************************************************
* %FUNCTION: session_compare
* %ARGUMENTS:
*  d1, d2 -- two sessions
* %RETURNS:
*  0 if sids are equal, non-zero otherwise
* %DESCRIPTION:
*  Compares two sessions
***********************************************************************/
static int session_compare(void *d1, void *d2)
{
	return ((l2tp_session *) d1)->my_id != ((l2tp_session *) d2)->my_id;
}

/**********************************************************************
* %FUNCTION: l2tp_session_hash_init
* %ARGUMENTS:
*  tab -- hash table
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Initializes hash table of sessions
***********************************************************************/
void l2tp_session_hash_init(hash_table *tab)
{
	hash_init(tab, offsetof(l2tp_session, hash_by_my_id), session_compute_hash, session_compare);
}

/**********************************************************************
* %FUNCTION: session_set_state
* %ARGUMENTS:
*  session -- the session
*  state -- new state
* %RETURNS:
*  Nothing
***********************************************************************/
static void session_set_state(l2tp_session *session, int state)
{
	if (state == session->state)
	{
		return;
	}
	DBG(l2tp_db(DBG_SESSION, "session(%s) state %s -> %s\n", l2tp_debug_session_to_str(session), state_names[session->state], state_names[state]));
	session->state = state;
}

/**********************************************************************
* %FUNCTION: l2tp_session_free
* %ARGUMENTS:
*  ses -- a session to free
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Frees a session, closing down all resources associated with it.
***********************************************************************/
void l2tp_session_free(l2tp_session *ses, char const *reason)
{
	session_set_state(ses, SESSION_IDLE);
	DBG(l2tp_db(DBG_SESSION, "session_free(%s) %s\n", l2tp_debug_session_to_str(ses), reason));
	close_ppp_session(ses, reason);
	memset(ses, 0, sizeof(l2tp_session));
	free(ses);
	TimeToExit();				// tell main code we're done now
}

/**********************************************************************
* %FUNCTION: session_make_sid
* %ARGUMENTS:
*  tunnel -- an L2TP tunnel
* %RETURNS:
*  An unused random session ID
***********************************************************************/
static uint16_t session_make_sid(l2tp_tunnel *tunnel)
{
	uint16_t sid;
	while(1)
	{
		L2TP_RANDOM_FILL(sid);
		if (sid)
		{
			if (!l2tp_tunnel_find_session(tunnel, sid))
			{
				return sid;
			}
		}
	}
}

/**********************************************************************
* %FUNCTION: l2tp_session_call_lns
* %ARGUMENTS:
*  peer -- L2TP peer
*  calling_number -- calling phone number (or MAC address or whatever...)
*  privateData -- private data to be stored in session structure
* %RETURNS:
*  A newly-allocated session, or NULL if session could not be created
* %DESCRIPTION:
*  Initiates session setup.  Once call is active, established() will be
*  called.
***********************************************************************/
l2tp_session *l2tp_session_call_lns(l2tp_peer *peer, char const *calling_number, EventSelector *es, void *privateData)
{
	l2tp_session *ses;
	l2tp_tunnel *tunnel;

	// Find a tunnel to the peer
	tunnel = l2tp_tunnel_find_for_peer(peer, es);
	if (!tunnel)
	{
		return NULL;
	}

	// Allocate session
	ses = (l2tp_session *)malloc(sizeof(l2tp_session));
	if (!ses)
	{
		l2tp_set_errmsg("session_call_lns: out of memory");
		return NULL;
	}

	// Init fields
	memset(ses, 0, sizeof(l2tp_session));
	ses->tunnel = tunnel;
	ses->my_id = session_make_sid(tunnel);
	ses->state = SESSION_WAIT_TUNNEL;
	strncpy(ses->calling_number, calling_number, MAX_HOSTNAME);
	ses->calling_number[MAX_HOSTNAME-1] = 0;
	ses->privateData = privateData;

	// Add it to the tunnel
	l2tp_tunnel_add_session(ses);

	return ses;
}

/**********************************************************************
* %FUNCTION: l2tp_session_notify_tunnel_open
* %ARGUMENTS:
*  ses -- an L2TP session
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Called when tunnel has been established
***********************************************************************/
void l2tp_session_notify_tunnel_open(l2tp_session *ses)
{
	uint16_t u16;
	uint32_t u32;
	l2tp_dgram *dgram;
	l2tp_tunnel *tunnel = ses->tunnel;

	if (ses->state != SESSION_WAIT_TUNNEL)
	{
		return;
	}

	// Send ICRQ
	DBG(l2tp_db(DBG_SESSION, "Session %s tunnel open\n", l2tp_debug_session_to_str(ses)));

	dgram = l2tp_dgram_new_control(MESSAGE_ICRQ, ses->tunnel->assigned_id, 0);
	if (!dgram)
	{
		l2tp_set_errmsg("Could not establish session - out of memory");
		l2tp_tunnel_delete_session(ses, "Out of memory");
		return;
	}

	// assigned session ID
	u16 = htons(ses->my_id);
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u16), VENDOR_IETF, AVP_ASSIGNED_SESSION_ID, &u16);

	// Call serial number
	u32 = htonl(call_serial_number);
	call_serial_number++;
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u32), VENDOR_IETF, AVP_CALL_SERIAL_NUMBER, &u32);

	// Ship it out
	l2tp_tunnel_xmit_control_message(tunnel, dgram);

	session_set_state(ses, SESSION_WAIT_REPLY);
}

/**********************************************************************
* %FUNCTION: l2tp_session_send_CDN
* %ARGUMENTS:
*  ses -- which session to terminate
*  result_code -- result code
*  error_code -- error code
*  fmt -- printf-style format string for error message
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Sends CDN with specified result code and message.
***********************************************************************/
void l2tp_session_send_CDN(l2tp_session *ses, int result_code, int error_code, char const *fmt, ...)
{
	char buf[256];
	va_list ap;
	l2tp_tunnel *tunnel = ses->tunnel;
	uint16_t len;
	l2tp_dgram *dgram;
	uint16_t u16;

	// Build the buffer for the result-code AVP
	buf[0] = result_code / 256;
	buf[1] = result_code & 255;
	buf[2] = error_code / 256;
	buf[3] = error_code & 255;

	va_start(ap, fmt);
	vsnprintf(buf+4, 256-4, fmt, ap);
	buf[255] = 0;
	va_end(ap);

	DBG(l2tp_db(DBG_SESSION, "session_send_CDN(%s): %s\n", l2tp_debug_session_to_str(ses), buf+4));

	len = 4 + strlen(buf+4);
	// Build the datagram
	dgram = l2tp_dgram_new_control(MESSAGE_CDN, tunnel->assigned_id, ses->assigned_id);

	if (!dgram)
	{
		return;
	}

	// Add assigned session ID
	u16 = htons(ses->my_id);
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u16), VENDOR_IETF, AVP_ASSIGNED_SESSION_ID, &u16);

	// Add result code
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, len, VENDOR_IETF, AVP_RESULT_CODE, buf);

	// TODO: Clean up
	session_set_state(ses, SESSION_IDLE);

	// Ship it out
	l2tp_tunnel_xmit_control_message(tunnel, dgram);

	// Free session
	l2tp_tunnel_delete_session(ses, buf+4);
}

/**********************************************************************
* %FUNCTION: l2tp_session_handle_CDN
* %ARGUMENTS:
*  ses -- the session
*  dgram -- the datagram
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Handles a CDN by destroying session
***********************************************************************/
void l2tp_session_handle_CDN(l2tp_session *ses, l2tp_dgram *dgram)
{
	char buf[1024];
	unsigned char *val;
	uint16_t len;

	val = l2tp_dgram_search_avp(dgram, ses->tunnel, NULL, NULL, &len, VENDOR_IETF, AVP_RESULT_CODE);
	if (!val || len < 4)
	{
		l2tp_tunnel_delete_session(ses, "Received CDN");
	}
	else
	{
		uint16_t result_code, error_code;
		char *msg;
		result_code = ((uint16_t) val[0]) * 256 + (uint16_t) val[1];
		error_code = ((uint16_t) val[2]) * 256 + (uint16_t) val[3];
		if (len > 4)
		{
			msg = (char *) &val[4];
		}
		else
		{
			msg = "";
		}
		snprintf(buf, sizeof(buf), "Received CDN: result-code = %d, error-code = %d, message = '%.*s'", result_code, error_code, (int) len-4, msg);
		buf[1023] = 0;
		l2tp_tunnel_delete_session(ses, buf);
	}
}

/**********************************************************************
* %FUNCTION: l2tp_session_handle_ICRP
* %ARGUMENTS:
*  ses -- the session
*  dgram -- the datagram
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Handles an ICRP
***********************************************************************/
void l2tp_session_handle_ICRP(l2tp_session *ses, l2tp_dgram *dgram)
{
	uint16_t u16;
	unsigned char *val;
	uint16_t len;
	uint32_t u32;

	int mandatory, hidden;
	l2tp_tunnel *tunnel = ses->tunnel;

	// Get assigned session ID
	val = l2tp_dgram_search_avp(dgram, tunnel, &mandatory, &hidden, &len, VENDOR_IETF, AVP_ASSIGNED_SESSION_ID);
	if (!val)
	{
		l2tp_set_errmsg("No assigned session-ID in ICRP");
		return;
	}
	if (!l2tp_dgram_validate_avp(VENDOR_IETF, AVP_ASSIGNED_SESSION_ID, len, mandatory))
	{
		l2tp_set_errmsg("Invalid assigned session-ID in ICRP");
		return;
	}

	// Set assigned session ID
	u16 = ((uint16_t) val[0]) * 256 + (uint16_t) val[1];

	if (!u16)
	{
		l2tp_set_errmsg("Invalid assigned session-ID in ICRP");
		return;
	}

	ses->assigned_id = u16;

	// If state is not WAIT_REPLY, fubar
	if (ses->state != SESSION_WAIT_REPLY)
	{
		l2tp_session_send_CDN(ses, RESULT_FSM_ERROR, 0, "Received ICRP for session in state %s", state_names[ses->state]);
		return;
	}

	// Tell PPP code that call has been established
	if (establish_ppp_session(ses) < 0)
	{
		l2tp_session_send_CDN(ses, RESULT_GENERAL_ERROR, ERROR_VENDOR_SPECIFIC, "Failed to establish session");
		return;
	}

	// Send ICCN
	dgram = l2tp_dgram_new_control(MESSAGE_ICCN, tunnel->assigned_id, ses->assigned_id);
	if (!dgram)
	{
		// Ugh... not much chance of this working...
		l2tp_session_send_CDN(ses, RESULT_GENERAL_ERROR, ERROR_OUT_OF_RESOURCES, "Out of memory");
		return;
	}

	// TODO: Speed, etc. are faked for now.

	// Connect speed
	u32 = htonl(57600);
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u32), VENDOR_IETF, AVP_TX_CONNECT_SPEED, &u32);

	// Framing Type
	u32 = htonl(1);
	l2tp_dgram_add_avp(dgram, tunnel, MANDATORY, sizeof(u32), VENDOR_IETF, AVP_FRAMING_TYPE, &u32);

	// Ship it out
	l2tp_tunnel_xmit_control_message(tunnel, dgram);

	// Set session state
	session_set_state(ses, SESSION_ESTABLISHED);
	ses->tunnel->peer->fail = 0;

}

/**********************************************************************
* %FUNCTION: l2tp_session_handle_ICCN
* %ARGUMENTS:
*  ses -- the session
*  dgram -- the datagram
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Handles an ICCN
***********************************************************************/
void l2tp_session_handle_ICCN(l2tp_session *ses, l2tp_dgram *dgram)
{
	l2tp_session_send_CDN(ses, RESULT_FSM_ERROR, 0, "No calls accepted");
}

/**********************************************************************
* %FUNCTION: l2tp_session_state_name
* %ARGUMENTS:
*  ses -- the session
* %RETURNS:
*  The name of the session's state
***********************************************************************/
char const *l2tp_session_state_name(l2tp_session *ses)
{
	return state_names[ses->state];
}

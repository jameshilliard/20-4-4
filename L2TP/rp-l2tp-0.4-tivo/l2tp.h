#define DBG(x) x

#define MAX_HOSTNAME			128	// maximum length of a hostname

#define MD5LEN					16	// Length of MD5 hash

// Debug bitmasks
#define DBG_TUNNEL				1	// Tunnel-related events
#define DBG_XMIT_RCV			2	// Datagram transmission/reception
#define DBG_AUTH				4	// Authentication
#define DBG_SESSION				8	// Session-related events
#define DBG_FLOW				16	// Flow control code
#define DBG_AVP					32	// Hiding/showing of AVP's
#define DBG_SNOOP				64	// Snooping in on LCP

// Maximum size of L2TP datagram we accept... kludge...
#define MAX_PACKET_LEN			4096

#define MAX_SECRET_LEN			96
#define MAX_OPTS				64

#define MAX_RETRANSMISSIONS		5

#define EXTRA_HEADER_ROOM		32

// an L2TP datagram
struct l2tp_dgram
{
	uint16_t msg_type;						// Message type
	uint8_t bits;							// Options bits
	uint8_t version;						// Version
	uint16_t length;						// Length (opt)
	uint16_t tid;							// Tunnel ID
	uint16_t sid;							// Session ID
	uint16_t Ns;							// Ns (opt)
	uint16_t Nr;							// Nr (opt)
	uint16_t off_size;						// Offset size (opt)
	unsigned char data[MAX_PACKET_LEN];		// Data
	size_t last_random;						// Offset of last random vector AVP
	size_t payload_len;						// Payload len (not including L2TP header)
	size_t cursor;							// Cursor for adding/stripping AVP's
	size_t alloc_len;						// Length allocated for data
	l2tp_dgram *next;						// Link to next packet in xmit queue
};

// An L2TP peer
struct l2tp_peer
{
	struct sockaddr_in addr;				// Peer's address
	char secret[MAX_SECRET_LEN];			// Secret for this peer
	size_t secret_len;						// Length of secret
	bool hide_avps;							// false by default (we never turn this on)
	int fail;								// Number of failed attempts.
};

// An L2TP tunnel
struct l2tp_tunnel
{
	hash_bucket hash_by_my_id;				// Hash bucket for tunnel hash table
	hash_bucket hash_by_peer;				// Hash bucket for tunnel-by-peer table
	hash_table sessions_by_my_id;			// Sessions in this tunnel
	uint16_t my_id;							// My tunnel ID
	uint16_t assigned_id;					// ID assigned by peer
	l2tp_peer *peer;						// The L2TP peer
	struct sockaddr_in peer_addr;			// Peer's address
	uint16_t Ns;							// Sequence of next packet to queue
	uint16_t Ns_on_wire;					// Sequence of next packet to be sent on wire
	uint16_t Nr;							// Expected sequence of next received packet
	uint16_t peer_Nr;						// Last packet ack'd by peer
	int ssthresh;							// Slow-start threshold
	int cwnd;								// Congestion window
	int cwnd_counter;						// Counter for incrementing cwnd in congestion-avoidance phase
	int timeout;							// Retransmission timeout (seconds)
	int retransmissions;					// Number of retransmissions
	int rws;								// Our receive window size
	int peer_rws;							// Peer receive window size
	EventSelector *es;						// The event selector
	EventHandler *hello_handler;			// Timer for sending HELLO
	EventHandler *timeout_handler;			// Handler for timeout
	EventHandler *ack_handler;				// Handler for sending Ack
	l2tp_dgram *xmit_queue_head;			// Head of control transmit queue
	l2tp_dgram *xmit_queue_tail;			// Tail of control transmit queue
	l2tp_dgram *xmit_new_dgrams;			// dgrams which have not been transmitted
	char peer_hostname[MAX_HOSTNAME];		// Peer's host name
	unsigned char response[MD5LEN];			// Our response to challenge
	unsigned char expected_response[MD5LEN];	// Expected resp. to challenge
	int state;								// Tunnel state
};

// A session within a tunnel
struct l2tp_session
{
	hash_bucket hash_by_my_id;				// Hash bucket for session table
	l2tp_tunnel *tunnel;					// Tunnel we belong to
	uint16_t my_id;							// My ID
	uint16_t assigned_id;					// Assigned ID
	int state;								// Session state

	uint16_t Nr;							// Data sequence number
	uint16_t Ns;							// Data sequence number
	char calling_number[MAX_HOSTNAME];		// Calling number
	void *privateData;						// Private data for call-op's use
};

// Bit definitions
#define TYPE_BIT						0x80
#define LENGTH_BIT						0x40
#define SEQUENCE_BIT					0x08
#define OFFSET_BIT						0x02
#define PRIORITY_BIT					0x01
#define RESERVED_BITS					0x34
#define VERSION_MASK					0x0F
#define VERSION_RESERVED				0xF0

#define AVP_MANDATORY_BIT				0x80
#define AVP_HIDDEN_BIT					0x40
#define AVP_RESERVED_BITS				0x3C

#define MANDATORY						1
#define NOT_MANDATORY					0
#define HIDDEN							1
#define NOT_HIDDEN						0
#define VENDOR_IETF						0

#define AVP_MESSAGE_TYPE				0
#define AVP_RESULT_CODE					1
#define AVP_PROTOCOL_VERSION			2
#define AVP_FRAMING_CAPABILITIES		3
#define AVP_BEARER_CAPABILITIES			4
#define AVP_TIE_BREAKER					5
#define AVP_FIRMWARE_REVISION			6
#define AVP_HOST_NAME					7
#define AVP_VENDOR_NAME					8
#define AVP_ASSIGNED_TUNNEL_ID			9
#define AVP_RECEIVE_WINDOW_SIZE			10
#define AVP_CHALLENGE					11
#define AVP_Q931_CAUSE_CODE				12
#define AVP_CHALLENGE_RESPONSE			13
#define AVP_ASSIGNED_SESSION_ID			14
#define AVP_CALL_SERIAL_NUMBER			15
#define AVP_MINIMUM_BPS					16
#define AVP_MAXIMUM_BPS					17
#define AVP_BEARER_TYPE					18
#define AVP_FRAMING_TYPE				19
#define AVP_CALLED_NUMBER				21
#define AVP_CALLING_NUMBER				22
#define AVP_SUB_ADDRESS					23
#define AVP_TX_CONNECT_SPEED			24
#define AVP_PHYSICAL_CHANNEL_ID			25
#define AVP_INITIAL_RECEIVED_CONFREQ	26
#define AVP_LAST_SENT_CONFREQ			27
#define AVP_LAST_RECEIVED_CONFREQ		28
#define AVP_PROXY_AUTHEN_TYPE			29
#define AVP_PROXY_AUTHEN_NAME			30
#define AVP_PROXY_AUTHEN_CHALLENGE		31
#define AVP_PROXY_AUTHEN_ID				32
#define AVP_PROXY_AUTHEN_RESPONSE		33
#define AVP_CALL_ERRORS					34
#define AVP_ACCM						35
#define AVP_RANDOM_VECTOR				36
#define AVP_PRIVATE_GROUP_ID			37
#define AVP_RX_CONNECT_SPEED			38
#define AVP_SEQUENCING_REQUIRED			39

#define HIGHEST_AVP						39

#define MESSAGE_SCCRQ					1
#define MESSAGE_SCCRP					2
#define MESSAGE_SCCCN					3
#define MESSAGE_StopCCN					4
#define MESSAGE_HELLO					6

#define MESSAGE_OCRQ					7
#define MESSAGE_OCRP					8
#define MESSAGE_OCCN					9

#define MESSAGE_ICRQ					10
#define MESSAGE_ICRP					11
#define MESSAGE_ICCN					12

#define MESSAGE_CDN						14
#define MESSAGE_WEN						15
#define MESSAGE_SLI						16

// A fake type for our own consumption
#define MESSAGE_ZLB						32767

// Result and error codes
#define RESULT_GENERAL_REQUEST			1
#define RESULT_GENERAL_ERROR			2
#define RESULT_CHANNEL_EXISTS			3
#define RESULT_NOAUTH					4
#define RESULT_UNSUPPORTED_VERSION		5
#define RESULT_SHUTTING_DOWN			6
#define RESULT_FSM_ERROR				7

#define ERROR_OK						0
#define ERROR_NO_CONTROL_CONNECTION		1
#define ERROR_BAD_LENGTH				2
#define ERROR_BAD_VALUE					3
#define ERROR_OUT_OF_RESOURCES			4
#define ERROR_INVALID_SESSION_ID		5
#define ERROR_VENDOR_SPECIFIC			6
#define ERROR_TRY_ANOTHER				7
#define ERROR_UNKNOWN_AVP_WITH_M_BIT	8

// Tunnel states
enum
{
	TUNNEL_IDLE,
	TUNNEL_WAIT_CTL_REPLY,
	TUNNEL_WAIT_CTL_CONN,
	TUNNEL_ESTABLISHED,
	TUNNEL_RECEIVED_STOP_CCN,
	TUNNEL_SENT_STOP_CCN
};

// Session states
enum
{
	SESSION_IDLE,
	SESSION_WAIT_TUNNEL,
	SESSION_WAIT_REPLY,
	SESSION_WAIT_CONNECT,
	SESSION_ESTABLISHED
};

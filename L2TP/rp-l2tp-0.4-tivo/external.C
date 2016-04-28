// Handle calling the external program, and
// HDLC conversions (see RFC1662).
//
// HDLC conversions are handled here so that the external command
// program can just pass a byte stream instead of a frame stream.
// This allows us to avoid having to mess with line discipline, and
// generally makes everything simpler (although a little less efficient).
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

#define MAX_FDS				256
#define MAX_ARGS			256

#define MAX_PPP_MRU			MAX_PACKET_LEN			// maximum supported MRU length
#define MAX_PPP_FRAME		(2+2+MAX_PPP_MRU)		// maximum frame length (address/control+protocol+MAX_PPP_MRU)

// Used to assemble frames from incoming serial HDLC data
struct frame_assembler
{
	bool started;				// tells if flags byte has been encountered
	bool in_escape;				// tells if the last byte was an escape
	unsigned int index;			// next place to write data into the frame
	unsigned char header_room[EXTRA_HEADER_ROOM];	// must be directly before the frame_buffer field -- allows room at start so L2TP can add stuff w/o copying
	unsigned char frame_buffer[MAX_PPP_FRAME+2];	// frame data buffer, two bytes at end allow for FCS to get collected before we realize we've hit the end
};

// The slave process
struct slave
{
	EventSelector *es;			// Event selector
	l2tp_session *ses;			// L2TP session we're hooked to
	pid_t pid;					// PID of child PPPD process
	int fd;						// File descriptor for event-handler loop
	bool had_output;			// used to avoid putting HDLC flags byte at start and end needlessly
	frame_assembler assembler;	// used to assemble frames from incoming HDLC data
	EventHandler *event;		// Event handler
};

static char external_command[4096];		// the command to run
static char *ext_argv[MAX_ARGS];		// external command broken down into argument pointers
static int ext_argc;

static const unsigned short fcs_table[256]=
	{
		0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
		0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
		0x1081, 0x0108, 0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E,
		0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876,
		0x2102, 0x308B, 0x0210, 0x1399, 0x6726, 0x76AF, 0x4434, 0x55BD,
		0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5,
		0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E, 0x54B5, 0x453C,
		0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD, 0xC974,
		0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB,
		0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3,
		0x5285, 0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A,
		0xDECD, 0xCF44, 0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72,
		0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9,
		0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5, 0xA96A, 0xB8E3, 0x8A78, 0x9BF1,
		0x7387, 0x620E, 0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738,
		0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862, 0x9AF9, 0x8B70,
		0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E, 0xF0B7,
		0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
		0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036,
		0x18C1, 0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E,
		0xA50A, 0xB483, 0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5,
		0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD,
		0xB58B, 0xA402, 0x9699, 0x8710, 0xF3AF, 0xE226, 0xD0BD, 0xC134,
		0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C,
		0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1, 0xA33A, 0xB2B3,
		0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72, 0x3EFB,
		0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232,
		0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A,
		0xE70E, 0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1,
		0x6B46, 0x7ACF, 0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9,
		0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330,
		0x7BC7, 0x6A4E, 0x58D5, 0x495C, 0x3DE3, 0x2C6A, 0x1EF1, 0x0F78,
	};

static unsigned short fcs_iterate(unsigned short fcs,const unsigned char *bytes,unsigned int length)
// Run an iteration of the FCS calculation over the passed bytes.
// The passed 'fcs' value will seed the calculation.
{
	while(length)
	{
		fcs=(fcs>>8)^fcs_table[(fcs^(*bytes))&0xFF];
		bytes++;
		length--;
	}
	return fcs;
}

static unsigned short generate_fcs(const unsigned char *frame,unsigned int frame_length)
// Generate an FCS 16 value for the given frame.
{
	unsigned short result;

	result=fcs_iterate(0xFFFF,frame,frame_length);
	result^=0xFFFF;									// complement result
	return result;
}

/**********************************************************************
* %FUNCTION: pty_get
* %ARGUMENTS:
*  mfp -- pointer to master file descriptor
*  sfp -- pointer to slave file descriptor
* %RETURNS:
*  0 on success, -1 on failure
* %DESCRIPTION:
*  Opens a PTY and sets line discipline to N_HDLC for ppp.
*  Taken almost verbatim from Linux pppd code.
***********************************************************************/
static int pty_get(int *mfp, int *sfp)
{
	char pty_name[24];
	struct termios tios;
	int mfd, sfd;

	mfd = -1;
	sfd = -1;

	mfd = open("/dev/ptmx", O_RDWR);
	if (mfd >= 0)
	{
		int ptn;
		if (ioctl(mfd, TIOCGPTN, &ptn) >= 0)
		{
			snprintf(pty_name, sizeof(pty_name), "/dev/pts/%d", ptn);
			ptn = 0;
			if (ioctl(mfd, TIOCSPTLCK, &ptn) < 0)
			{
				// warn("Couldn't unlock pty slave %s: %m", pty_name);
			}
			if ((sfd = open(pty_name, O_RDWR | O_NOCTTY)) < 0)
			{
				// warn("Couldn't open pty slave %s: %m", pty_name);
			}
		}
	}

	if (sfd < 0 || mfd < 0)
	{
		if (sfd >= 0) close(sfd);
		if (mfd >= 0) close(mfd);
		return -1;
	}

	*mfp = mfd;
	*sfp = sfd;
	if (tcgetattr(sfd, &tios) == 0)
	{
		tios.c_cflag &= ~(CSIZE | CSTOPB | PARENB);
		tios.c_cflag |= CS8 | CREAD | CLOCAL;
		tios.c_iflag  = IGNPAR;
		tios.c_oflag  = 0;
		tios.c_lflag  = 0;
		tcsetattr(sfd, TCSAFLUSH, &tios);
	}
	return 0;
}

/**********************************************************************
* %FUNCTION: slave_exited
* %ARGUMENTS:
*  pid -- PID of exiting slave
*  status -- exit status of slave
*  data -- the slave structure
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Handles an exiting slave
***********************************************************************/
static void slave_exited(pid_t pid, int status, void *data)
{
	l2tp_session *ses;
	struct slave *sl = (struct slave *) data;

	if (sl)
	{
		ses = sl->ses;

		if (sl->fd >= 0)
		{
			close(sl->fd);
		}
		if (sl->event)
		{
			Event_DelHandler(sl->es, sl->event);
		}

		if (ses)
		{
			ses->privateData = NULL;
			l2tp_session_send_CDN(ses, RESULT_GENERAL_REQUEST, 0, "pppd process exited");
		}
		free(sl);
	}
}

static unsigned int write_hdlc_byte(unsigned char *hdlc_generation_buffer,unsigned int index,unsigned char byte)
// Add byte into hdlc_generation_buffer at index, escaping it as needed.
// Return the number of bytes generated.
{
	unsigned int num_bytes;

	if((byte<0x20)||(byte==0x7D)||(byte==0x7E))		// see if the byte needs to be escaped
	{
		hdlc_generation_buffer[index]=0x7D;			// write escape code
		hdlc_generation_buffer[index+1]=byte^0x20;	// write escaped value
		num_bytes=2;
	}
	else
	{
		hdlc_generation_buffer[index]=byte;			// byte can just be copied over
		num_bytes=1;
	}
	return num_bytes;
}

static unsigned int generate_hdlc(const unsigned char *frame,unsigned int frame_length,unsigned char *hdlc_generation_buffer,bool had_output)
// Convert a frame into an HDLC encoded sequence of
// bytes which can be sent out to the client serially.
// hdlc_generation_buffer must be large enough to hold the
// maximum number of bytes this can generate:
// 1+(frame_length+2)*2+1
// Returns the number of bytes generated.
{
	unsigned int offset;
	unsigned int i;
	unsigned short fcs;

	offset=0;
	if(!had_output)
	{
		hdlc_generation_buffer[offset++]=0x7E;		// drop a "Flag" byte into the frame to start it off if it is the first-ever frame
	}

	for(i=0;i<frame_length;i++)
	{
		offset+=write_hdlc_byte(hdlc_generation_buffer,offset,frame[i]);	// output all the bytes of the frame, escaping as needed
	}

	fcs=generate_fcs(frame,frame_length);			// work out the FCS 16 value to append

	offset+=write_hdlc_byte(hdlc_generation_buffer,offset,fcs&0xFF);		// write the FCS value out (little endian)
	offset+=write_hdlc_byte(hdlc_generation_buffer,offset,(fcs>>8)&0xFF);

	hdlc_generation_buffer[offset++]=0x7E;			// stick on flag to mark the end of the frame (and start of the next)

	return offset;
}

/**********************************************************************
* %FUNCTION: handle_ppp_frame
* %ARGUMENTS:
*  ses -- l2tp session
*  buf -- received PPP frame
*  len -- length of frame
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Shoots the frame to PPP's pty
***********************************************************************/
void handle_ppp_frame(l2tp_session *ses, unsigned char *buf, size_t len)
{
	unsigned char hdlc_generation_buffer[1+(len+2)*2+1];
	slave *sl = (slave *)ses->privateData;
	int n;

	if (sl)
	{
		// Add framing bytes
		*--buf = 0x03;
		*--buf = 0xFF;
		len += 2;

		len=generate_hdlc(buf,len,hdlc_generation_buffer,sl->had_output);
		sl->had_output=true;		// remember we've generated a frame -- no more need for flags byte at the start

		// TODO: Add error checking
		n = write(sl->fd, hdlc_generation_buffer, len);
	}
}

/**********************************************************************
* %FUNCTION: close_session
* %ARGUMENTS:
*  ses -- L2TP session
*  reason -- reason why session is closing
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Kills pppd.
***********************************************************************/
void close_ppp_session(l2tp_session *ses, char const *reason)
{
	slave *sl = (slave *)ses->privateData;

	if (sl)
	{
		// Detach slave
		ses->privateData = NULL;
		sl->ses = NULL;

		DBG(l2tp_db(DBG_TUNNEL, "Sending TERM to command PID: %d\n",sl->pid));
		kill(sl->pid,SIGTERM);
		DBG(l2tp_db(DBG_TUNNEL, "Waiting for command to quit\n"));
		waitpid(sl->pid,NULL,0);			// wait for the child to have exited
		close(sl->fd);
		sl->fd = -1;
		Event_DelHandler(sl->es, sl->event);
		sl->event = NULL;
	}
}

static inline void reset_frame_assembler(frame_assembler *assembler,bool started)
// Reset the HDLC frame assembler. If started == true, we'll immediately
// begin collecting bytes into the frame. If false, we'll wait until the next
// "Flag" byte is seen.
{
	assembler->started=started;
	assembler->in_escape=false;				// have not seen escape character in the previous position
	assembler->index=0;						// reset the index
}

static inline void collect_hdlc_byte_into_frame(frame_assembler *assembler,unsigned char byte)
// Passed a byte of HDLC encoded data, add it to the growing frame.
// Check to make sure the frame is not getting too large. If it is, it
// means we're getting garbage, and we need to throw out what was collected.
{
	if(assembler->index<sizeof(assembler->frame_buffer))
	{
		assembler->frame_buffer[assembler->index++]=byte;
	}
	else
	{
		reset_frame_assembler(assembler,false);			// oops, frame got too large -- just reset and wait for another
	}
}

static void collect_hdlc_bytes(l2tp_session *ses,frame_assembler *assembler,const unsigned char *data,unsigned int data_length)
// When data arrives from the client, shove it through the frame assembler
// to build up complete frames (by stripping off the HDLC encoding).
// As each frame emerges from the process, send it out through the tunnel.
{
	unsigned short fcs;
	unsigned short frame_fcs;
	unsigned char byte;

	while(data_length)
	{
		byte=(*data++);

		if(assembler->started)
		{
			if(assembler->in_escape)
			{
				collect_hdlc_byte_into_frame(assembler,byte^0x20);// unescape the byte
				assembler->in_escape=false;				// reset escape flag
			}
			else if(byte==0x7D)
			{
				assembler->in_escape=true;				// set escape flag
			}
			else if(byte==0x7E)							// got end of HDLC frame (and/or start of the next)
			{
				if(assembler->index>=2+2)				// at least 2 bytes of frame, and an FCS
				{
					assembler->index-=2;
					fcs=generate_fcs(assembler->frame_buffer,assembler->index);
					frame_fcs=(assembler->frame_buffer[assembler->index])|(assembler->frame_buffer[assembler->index+1]<<8);
					if(fcs==frame_fcs)
					{
						l2tp_dgram_send_ppp_frame(ses,assembler->frame_buffer+2,assembler->index-2);	// Chop off framing bytes, send the frame
					}
				}
				reset_frame_assembler(assembler,true);	// reset for the next frame (which is allowed to start immediately)
			}
			else
			{
				collect_hdlc_byte_into_frame(assembler,byte);
			}
		}
		else	// not started, waiting for Flag byte
		{
			if(byte==0x7E)
			{
				reset_frame_assembler(assembler,true);	// got Flag, so start assembling this frame
			}
		}
		data_length--;
	}
}

/**********************************************************************
* %FUNCTION: readable
* %ARGUMENTS:
*  es -- event selector
*  fd -- file descriptor
*  flags -- we ignore
*  data -- the L2TP session
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Handles readability on PTY; shoots PPP frame over tunnel
***********************************************************************/
static void readable(EventSelector *es, int fd, unsigned int flags, void *data)
{
	unsigned char buf[4096];
	int n;
	l2tp_session *ses = (l2tp_session *) data;
	int iters = 5;
	slave *sl;
	frame_assembler *assembler;

	if(ses)
	{
		sl = (slave *)ses->privateData;
		assembler = &sl->assembler;

		// It seems to be better to read in a loop than to go
		// back to select loop.  However, don't loop forever, or
		// we could have a DoS potential
		while(iters--)
		{
			// EXTRA_HEADER_ROOM bytes extra space for l2tp header
			n = read(fd, buf, sizeof(buf));

			if(n>0)			// make sure read succeeded
			{
				collect_hdlc_bytes(ses,assembler,buf,n);
			}
			else
			{
				return;		// hit end of available data
			}
		}
	}
}

/**********************************************************************
* %FUNCTION: establish_ppp_session
* %ARGUMENTS:
*  ses -- the L2TP session
* %RETURNS:
*  0 if session could be established, -1 otherwise.
* %DESCRIPTION:
*  Forks a pppd process and connects fd to pty
***********************************************************************/
int establish_ppp_session(l2tp_session *ses)
{
	int m_pty, s_pty;
	pid_t pid;
	EventSelector *es = ses->tunnel->es;
	slave *sl = (slave *)malloc(sizeof(struct slave));
	int i;
	struct sigaction act;

	ses->privateData = NULL;
	if (!sl)
	{
		return -1;
	}
	sl->ses = ses;
	sl->es = es;

	// Get pty
	if (pty_get(&m_pty, &s_pty) < 0)
	{
		DBG(l2tp_db(DBG_TUNNEL, "Failed to get pty\n"));
		free(sl);
		return -1;
	}

	// Fork
	pid = fork();
	if (pid == (pid_t) -1)
	{
		free(sl);
		return -1;
	}

	if (pid)
	{
		int flags;

		// In the parent
		sl->pid = pid;
		DBG(l2tp_db(DBG_TUNNEL, "Tunnel command PID: %d\n",pid));

		reset_frame_assembler(&sl->assembler,false);	// initialize HDLC frame assembly
		sl->had_output=false;							// have never output an HDLC frame yet

		// Set up handler for when pppd exits
		Event_HandleChildExit(es, pid, slave_exited, sl);

		// Close the slave tty
		close(s_pty);

		sl->fd = m_pty;

		// Set slave FD non-blocking
		flags = fcntl(sl->fd, F_GETFL);
		if (flags >= 0)
		{
			fcntl(sl->fd, F_SETFL, (long) flags | O_NONBLOCK);
		}

		// Handle readability on slave end
		sl->event = Event_AddHandler(es, m_pty, EVENT_FLAG_READABLE, readable, ses);

		ses->privateData = sl;
		return 0;
	}

	// In the child.  Exec pppd
	// Close all file descriptors except s_pty
	for (i=0; i<MAX_FDS; i++)
	{
		if (i != s_pty)
		{
			close(i);
		}
	}

	// Dup s_pty onto stdin and stdout
	dup2(s_pty, 0);
	dup2(s_pty, 1);
	if (s_pty > 1)
	{
		close(s_pty);
	}

	memset(&act,0x00,sizeof(act));
	act.sa_handler=SIG_DFL;			// restore these signals to their default behavior
	sigaction(SIGINT,&act,NULL);
	sigaction(SIGTERM,&act,NULL);

    execv(ext_argv[0],ext_argv);	// run the command

	// Doh.. execl failed
	_exit(1);
}

static void skip_whitespace(const char *string,int *index)
// Push index forward until string[index] points to a \0,
// or a non whitespace character.
{
	while(string[*index]&&isspace(string[*index]))
	{
		(*index)++;
	}
}

static void skip_nonwhitespace(const char *string,int *index)
// Push index forward until string[index] points to a \0,
// whitespace character.
{
	while(string[*index]&&!isspace(string[*index]))
	{
		(*index)++;
	}
}

void set_ppp_session_command(const char *command)
// Set the command string that will be run when the tunnel has been established.
{
	int index;

	strncpy(external_command,command,sizeof(external_command));
	external_command[sizeof(external_command)-1]='\0';

	// run through and make an argc, argv array out of the command
	ext_argc=0;
	index=0;

	skip_whitespace(external_command,&index);			// step over initial whitespace
	do
	{
		ext_argv[ext_argc++]=&external_command[index];	// point at the next thing on the line
		skip_nonwhitespace(external_command,&index);	// step over it
		if(external_command[index])						// make sure we've not hit the end of the line
		{
			external_command[index++]='\0';				// terminate the argument, and skip over the termination character
			skip_whitespace(external_command,&index);	// step over any remaining whitespace
		}
	}
	while(external_command[index]&&ext_argc<(MAX_ARGS-1));	// run until hit end of command, or no more room for aguments

	ext_argv[ext_argc]=NULL;							// terminate the argv array
}


#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/telnet.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <time.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sched.h>
#include <wait.h>

#undef DEBUG
#define BUFSIZE 512
#define RTPRI 26

typedef struct _conn {
     int pty;
     int network;
     char name[15];
     pid_t pid;
     struct in_addr ineta;
     struct _conn *next;
     struct pollfd polldat;
} conn;

void tn_init(int incoming);
void tn_recv_opt(unsigned char *opt, int net, conn *client, int optlen);
void tn_subneg(unsigned char *opt, int net, conn *client, struct winsize *lm, int optlen);
int tn_simple_opt(unsigned char opt, unsigned char state, int net);
void kiddie_clean(int sig);
void finish(int sig);
void clean_buffer(char *buffer, int size);
int pty_open(char *pts_name);
conn *start_pty(int netsock, int ineta, int stay_rt);
int end_pty(conn *connected);
conn *try_new(int netsock, int stay_rt);
void eat_input(int fd);
void close_connection( conn *conection);
void print_conn( conn * connection );
conn *mend_connect( conn * connection );

int quit;
struct sockaddr_in sockad;
int socksize = sizeof(struct sockaddr_in);
conn *connects;
char **bin;

int main(int ac, char *av[]) {

    conn *connect1, *incoming = NULL, *connect2 = NULL, *prev = NULL;
    int port, port2;
    int mainsock, mainsock2;
    int x;
    int sockopts;
    unsigned char buffer[BUFSIZE];
    int len;
    int debug;
    struct sched_param sp;
    fd_set fds;
    int maxfd = 0;
    int sel = 0;

    {
        /*
         * Properly handle signals. Ignore SIGHUP - if the
         * process on the other side of a PTY dies, we'll
         * find out through SIGCHLD.
         */
        struct sigaction sa;
        sigemptyset( &sa.sa_mask );
        sa.sa_flags = 0;

        sa.sa_handler = kiddie_clean;
        sigaction( SIGCHLD, &sa, 0 );

        sa.sa_handler = finish;
        sigaction( SIGTERM, &sa, 0 );
        sigaction( SIGINT, &sa, 0 );

        sa.sa_handler = SIG_IGN;
        sigaction( SIGHUP, &sa, 0 );
    }

    connects = NULL;
    quit = 0;


    if(ac < 3) {
        printf("usage: %s <port> [-r <port2>] <program>\n", av[0]);
        exit(254);
    }

    if((port = atoi(av[1])) == 0) {
        printf("Please specify a valid decimal port number.\n");
        exit(127);
    }

    if(ac > 4 && strcmp(av[2],"-r") == 0) {
        if((port2 = atoi(av[3])) == 0) {
            printf("Please specify a valid decimal port number.\n");
            exit(127);
        }
        bin = av + 4;
    } else {
        port2 = 0;
        bin = av + 2;
    }

    /* begin to prepare our listening socket. */

    mainsock = socket(AF_INET, SOCK_STREAM, 0);

    sockad.sin_family = AF_INET;
    sockad.sin_port = htons(port);
    sockad.sin_addr.s_addr = inet_addr("0.0.0.0");

    x = 1; 
    if(setsockopt(mainsock, SOL_SOCKET, SO_REUSEADDR, &x, sizeof(x)) < 0) {
        printf("Unable to properly configure socket.\n");
        exit(127);
    }

    if(bind(mainsock, (struct sockaddr *)&sockad, socksize) != 0) {
        printf("Couldn't bind to socket.\n");
        exit(127);
    }

    if(listen(mainsock, 5) != 0) {
        printf("Couldn't listen on socket.\n");
        exit(127);
    }

    if(port2) {
        mainsock2 = socket(AF_INET, SOCK_STREAM, 0);

        sockad.sin_family = AF_INET;
        sockad.sin_port = htons(port2);
        sockad.sin_addr.s_addr = inet_addr("0.0.0.0");

        if(setsockopt(mainsock2, SOL_SOCKET, SO_REUSEADDR, &x, sizeof(x)) < 0) {
            printf("Unable to properly configure socket.\n");
            exit(127);
        }

        if(bind(mainsock2, (struct sockaddr *)&sockad, socksize) != 0) {
            printf("Couldn't bind to socket.\n");
            exit(127);
        }

        if(listen(mainsock2, 5) != 0) {
            printf("Couldn't listen on socket.\n");
            exit(127);
        }
    }

     /* set up for logging */
     openlog( "tnlited", 0, LOG_DAEMON );

     /* alright, now fork off if they're in not in debug mode. */
#ifndef DEBUG
     if(fork() != 0)
          exit (0);
#endif

     /* go to a high RT pri */
    sp.sched_priority = RTPRI;
    sched_setscheduler( 0, SCHED_FIFO, &sp );

    while(quit == 0) {
        
        /* initialize some variables */
        FD_ZERO(&fds);
        x = 0;
        FD_SET(mainsock, &fds); 
        maxfd = mainsock;

        if(port2) {
            FD_SET(mainsock2, &fds); 
            if(mainsock2 > maxfd)
                maxfd = mainsock2;
        }

        connect1 = connects;
        while(connect1 != NULL) {
            FD_SET(connect1->pty, &fds);
            if(connect1->pty > maxfd)
                maxfd = connect1->pty;
            FD_SET(connect1->network, &fds);
            if(connect1->network > maxfd)
                maxfd = connect1->network;
            connect1 = connect1->next;
        }
          
        sel = select(maxfd+1, &fds, NULL, NULL, NULL); 

        /* printf("select: %d\n", sel); */
        if(sel > 0) {
            connect1 = connects;
            while(connect1 != NULL) {
                // let's check that the connection's shell is still running
                // if not, let's kill the connection
#ifdef BAD_LAUNCHER_PTY
                if( waitpid(connect1->pid,NULL,WNOHANG) != 0 ) {
                    /* child died */
                    close_connection(connect1);

                    /* and now mend the connects */
                    connect1 = mend_connect(connect1);
                    continue;
                }
#endif
                if(FD_ISSET(connect1->pty, &fds)) {
                    //printf("there is data from the session to write to the remote user\n");
                    FD_CLR(connect1->pty, &fds);
                    len = read(connect1->pty, buffer, sizeof(buffer));
                    if( 0 == len ) {
                        /* child died. */
                        close_connection(connect1);
                             
                        /* and now mend the connects. */
                        connect1 = mend_connect( connect1 );
                        continue;
                    } else if( len < 0 ) {
                        int errsv = errno;
                        /* an error occured */
                        syslog(LOG_INFO, 
                               "Error %s reading pty from %s",
                               strerror(errsv),
                               connect1->name );
                        close_connection(connect1);
                        connect1 = mend_connect( connect1 );
                        continue;
                    } else {
                        write(connect1->network, buffer, len);
                    }
                    //printf("from pty read: %d\n", len);
                }
                if(FD_ISSET(connect1->network, &fds)) {
                    FD_CLR(connect1->network, &fds);
                    len = read(connect1->network, buffer, sizeof(buffer));
                    if( 0 == len ) {
                        /* No more data to read. */
                        close_connection(connect1);
                        
                        /* and now mend the connects. */     
                        connect1 = mend_connect( connect1 );
                        continue;
                    } else if(len < 0 ) {
                        int errsv = errno;
                        /* an error occured */
                        syslog(LOG_INFO, 
                               "Error %s reading network from %s",
                               strerror(errsv),
                               connect1->name );
                        close_connection(connect1);
                        connect1 = mend_connect( connect1 );
                        continue;
                    } else {
                        //printf("second len not less then 1: %d\n", len );
                        if(buffer[0] == IAC) {
                            tn_recv_opt(buffer,
                                connect1->network,
                                connect1,
                                len);
                            continue;
                        }
                        /* for some fucked up reason, the
                           terminals like to send a \0 after a
                           \n.  why, I don't know. */
                        if(buffer[len-1] == 0)
                            len--;
                        write(connect1->pty, buffer, len);
                    }
                }
                connect1 = connect1->next;
            }
               
            if(FD_ISSET(mainsock, &fds)) {
                if((incoming = try_new(mainsock, 0)) == NULL)
                    continue;
                else
                    connects = incoming;
            }
               
            if(port2) {
                if(FD_ISSET(mainsock2, &fds)) {
                    if((incoming = try_new(mainsock2, 1)) == NULL)
                        continue;
                    else
                        connects = incoming;
                }
            }
        }
    }
}

conn *try_new(int netsock, int stay_rt) {

     int incoming;
     static conn *ret;
     char buf[10];
     struct sockaddr_in insock;
     int socksize;

     socksize = sizeof(struct sockaddr);

     if((incoming = accept(netsock, (struct sockaddr *)&insock,
                           &socksize)) == -1) {
#ifdef DEBUG
          printf("nothing.  NULL back\n");
#endif
          return NULL;
     }
#ifdef DEBUG     
     printf("Connection on %d\n", incoming);
#endif
     syslog(LOG_INFO, "Accepted connection from %s\n",
         inet_ntoa(insock.sin_addr));

     /* send out some telnet commands to try to bang them into a
        happier mode.  with any luck. */

     ret = start_pty(incoming, insock.sin_addr.s_addr, stay_rt);
     if (ret == NULL)
         close(incoming);
     return ret;
 
}

/* opens up a pty, and fires off the BBS on it. */

conn *start_pty(int netsock, int ineta, int stay_rt) {

     conn *ret;
     char nawsing[50];
     int nread;
     int pend[2];
     char dummy[1];
     struct winsize ws = { 0, 0 };

     ret = malloc(sizeof(conn));
     
     /* allocate the pseudo-tty */
     ret->network = netsock;
     ret->pty = pty_open(ret->name);
     if(ret->pty == -1) {
          free(ret);
          return NULL;
     }
     ret->ineta.s_addr = ineta;
     ret->next = connects;
     syslog(LOG_INFO, "New connection from %s assigned pty %s", inet_ntoa(ret->ineta),
             ret->name);

#ifdef DEBUG
     printf("Negotiating telnet parameters\n");
#endif
     tn_init(netsock);
     if(tn_simple_opt(TELOPT_NAWS, DO, netsock) == 1) {
           nread = read(netsock, nawsing, 50);
           tn_subneg(nawsing, netsock, ret, &ws, nread);
     }
#ifdef DEBUG
     else {
          printf("Bad response to NAWS request, read data ignored\n");  
     }
#endif

     // This will clear out any pending junk on input, such as improperly
     // handled response packets
     //
     eat_input(netsock);

#ifdef DEBUG
     printf("Starting communication program\n");
#endif
     pipe(pend);
     if((ret->pid = fork()) == -1) {
          syslog(LOG_ERR, "%s/%s: unable to fork communication program",
                  inet_ntoa(ret->ineta), ret->name);
          free(ret);
          return(NULL);
     }

     if (ret->pid == 0) {
         struct termios tty_term;
         int ourtty;
         int i;
         int nfd = getdtablesize();
         struct sched_param sp;

#ifdef DEBUG
          printf("Child closing descriptors ...\n");
#endif
          close(pend[0]);
          for (i = 3; i < nfd; i++)
                if (i != pend[1])
                    close(i);

          if((ourtty = open(ret->name, O_RDWR)) == -1) {
#ifdef DEBUG
               printf("Child unable to open pty %s, err = %d\n", ret->name, errno);
#endif
               exit(0);
          }

          /* become our own session leader */
#ifdef DEBUG
          printf("setsid: %d\n", setsid());
#else
          setsid();
#endif

          /* become the controlling tty */
          if(ioctl(ourtty, TIOCSCTTY,  ourtty) == -1) {
               printf("fork TIOCSCTTY failed %d\n",errno);
               /* exit(0); */
          }

          /* fix up some termios bits. */
          if(tcgetattr(ourtty, &tty_term) == -1)
               printf("Couldn't get termios data.\n");
          tty_term.c_lflag |= NOFLSH;
          tty_term.c_lflag |= ISIG;
          if(tcsetattr(ourtty, TCSANOW, &tty_term) == -1)
               printf("Couldn't set termios data.\n");

          if (ws.ws_row != 0 && ws.ws_col != 0) {
                  if(ioctl(ourtty, TIOCSWINSZ, (char *)&ws) < 0)
                       printf("couldn't ioctl() out the window size.\n");
          }

          /* configure stdin/out/err */
          dup2(ourtty, 0);
          dup2(ourtty, 1);
          dup2(ourtty, 2);

          /* let parent go */
          close(pend[1]);

          if (stay_rt) {
               /* If we're staying at RT pri, go to a slightly lower RT pri,
                * and play well with other children */
               sp.sched_priority = RTPRI - 1;
               sched_setscheduler( 0, SCHED_RR, &sp );
          } else {
               /* otherwise drop down to non-RT */
               sp.sched_priority = 0;
               sched_setscheduler( 0, SCHED_OTHER, &sp );
          }

          execv(bin[0], bin);
          /* if we're here, there was an error. :( */
          syslog(LOG_ERR, "%s/%s: unable to exec communication program",
                  inet_ntoa(ret->ineta), ret->name);
          printf("exec \"%s\" failed, err = %d\n", bin[0], errno);
          exit(0);
     }
          
     /* wait for the child to be ready */
     close(pend[1]);
     read(pend[0], dummy, 1);
     close(pend[0]);

     return ret;
}

int end_pty(conn *connect) {
     char buffer[10];

#ifdef DEBUG
     printf("sending signal to %d\n", connect->pid);
#endif
     syslog(LOG_INFO, "%s/%s: connection closed", inet_ntoa(connect->ineta), connect->name);
     kill(connect->pid, SIGINT);
     buffer[0] = '\177';
     write(connect->pty, buffer, 1);
     sleep(2);

     return 0;
}

     
int pty_open(char *pts_name) {

     int ptyfd;
     char *ptr1, *ptr2;

     strcpy(pts_name, "/dev/ptyXY");

     /* now sweep for available pty devices... */
     /* this is the naming convention that my devices have in linux... */

     for(ptr1 = "pqrstuvwxyzabede" ; *ptr1 != 0 ; ptr1++) {
          pts_name[8] = *ptr1;
          for(ptr2 = "0123456789abcdef" ; *ptr2 != 0 ; ptr2++) {
               pts_name[9] = *ptr2;
               /* ok, try to actually open it */

               if((ptyfd = open(pts_name, O_RDWR)) < 0) {
                    if(errno == ENOENT)
                         break;
                    else
                         continue;
               }

               pts_name[5] = 't';
               return ptyfd;
          }
     }

     syslog(LOG_ERR, "No ptys available");
     return -1;  /* exhausted our search space. */

}

/* ok, kids.  It's bath time... */

void kiddie_clean(int sig) {
    wait(NULL);
}

/* do our shutdown routine for the telnet session */
void close_connection( conn * connection ) {
    shutdown(connection->network, 2);
    close(connection->pty);
    end_pty(connection);
    close(connection->network);
}


void finish(int sig) {

     conn *cleanme;
     
     cleanme = connects;
     while(cleanme != NULL) {
          end_pty(cleanme);
          shutdown(cleanme->network, 2);
          close(cleanme->network);
          close(cleanme->pty);
          cleanme = cleanme->next;
     }

     exit(0);
}

     
void tn_init(int incoming) {

     char xmit[10];
     char in[30];
     int nread;
     
     tn_simple_opt(TELOPT_ECHO, WILL, incoming);
     tn_simple_opt(TELOPT_SGA, WILL,incoming);  
     tn_simple_opt(TELOPT_SGA, DO, incoming);

     if(tn_simple_opt(TELOPT_TTYPE, DO, incoming) == 1) {
          xmit[0] = IAC;
          xmit[1] = SB;
          xmit[2] = TELOPT_TTYPE;
          xmit[3] = TELQUAL_SEND;
          xmit[4] = IAC;
          xmit[5] = SE;
          write(incoming, xmit, 6);
          nread = read(incoming, in, 30);
          tn_subneg(in, incoming, NULL, 0, nread);
     }
          
     return;
     

}

/* this handles what we do when the other end sends us a request to do
   something with a telnet option. */

void tn_recv_opt(unsigned char *opt, int net, conn *client, int optlen) {

     unsigned char resp[5];

     resp[0] = IAC;
     
     if(opt[0] != IAC)
          return;
     
     switch(opt[1]) {
     case DO:
     case DONT:
          resp[1] = WONT;
          resp[2] = opt[2];
          write(net, resp, 3);
          return;
     case WILL:
     case WONT:
          resp[1] = DONT;
          resp[2] = opt[2];
          write(net, resp, 3);
          return;
     case SB:
          tn_subneg(opt, net, client, 0, optlen);
          return;


     }
}

/* subnegotiates a given telnet option.  moved here for clarity. */
/* we need to have the connection sent in case we need to SIGWINCH
   it. */

void tn_subneg(unsigned char *opt, int net, conn *client, struct winsize *lm, int optlen) {
     
     /* naws vars */
     struct winsize winmod;
     /* termtype's */
     char term[20];
     char termbreak[2] = {255 , 0};
     char *tbloc;
     unsigned char *optend = opt + optlen - 4;

     while (1) {

          while((opt[0] != IAC) || (opt[1] != SB)) {
               if (++opt > optend) {
                    return;
               }
          }

          switch(opt[2]) {
          case TELOPT_NAWS:
          /* time to modify the window size. */
               opt += 3;
               winmod.ws_row = opt[3];
               winmod.ws_col = opt[1];
               opt += 6;
#ifdef DEBUG
               printf("NAWSing..  %dx%d\n", winmod.ws_col, winmod.ws_row);
#endif
               if (lm) {
                    lm->ws_row = winmod.ws_row;
                    lm->ws_col = winmod.ws_col;
               } else if (client) {
                    if(ioctl(client->pty, TIOCSWINSZ, (char *)&winmod) < 0)
                         printf("couldn't ioctl() out the window size.\n");
                    kill(client->pid, SIGWINCH);
               }
               break;
          case TELOPT_TTYPE:
               /* they want to tell us what kind of terminal they've got */
               opt += 4;
               tbloc = strsep(&opt, termbreak);
               if (!tbloc) {
                    break;
               }
               strcpy(term, tbloc);
#ifdef DEBUG
               printf("they claim to have a %s terminal\n", term);
#endif 
               /* TiVo hack: only one entry in termcap, use that... */
               setenv("TERM", "xterm", 1);
               if (!opt) {
                    return;
               }
               opt += 1;
               break;
#ifdef DEBUG
          default:
               printf("Unhandled TELOPT: %d\n", opt[2]);
               break;
#endif
          }

     }

}

/* sends out an option which doesn't requre subnegotiation. */
/* state describes the type of change to make. */

int tn_simple_opt(unsigned char opt, unsigned char state, int net) {

     unsigned char xmit[5];
     unsigned char inbound[5];
     
     xmit[0] = IAC;
     xmit[1] = state;
     xmit[2] = opt;
     
     write(net, xmit, 3);
     if (read(net, inbound, 3) != 3)
          return -1;
     if(inbound[0] != IAC)
          return -1;
     // This is completely broken: should also check that inbound[2] == opt,
     // but that would just expose further brokenness of this approach...
     switch(state) {
     case DO:
     case DONT:
          if(inbound[1] == WILL)
               return 1;
          else if(inbound[1] == WONT)
               return 0;
          else 
               return -1;
     case WILL:
     case WONT:
          if(inbound[1] == DO)
               return 1;
          else if(inbound[1] == DONT)
               return 0;
          else
               return -1;
     
     }

     /* if we're here, something's fucked up. */

     return -1;
     
}

void print_conn( conn * connection )
{
    printf( "connection %s:\n\tpty: %d\n\tnetwork: %d\n\tpid: %d\n",
            connection->name, connection->pty, connection->network, connection->pid );
}

conn *mend_connect( conn * connection )
{
    conn * con2;
    conn * prev = NULL;
    
    con2 = connects;
    while(con2 != connection) {
        prev = con2;
        con2 = con2->next;
    }
    if( NULL == prev ) {
        connects = con2->next;
    } else {
        prev->next = con2->next;
    }
    free(con2);
    connection = connection->next;
    return connection;
}

void eat_input(int fd)
{
     struct timeval timeout;
     fd_set fds;
     unsigned char buff[20];

     timeout.tv_sec = 0;
     timeout.tv_usec = 0;
     FD_ZERO(&fds);
     FD_SET(fd, &fds);
     while (select(fd+1, &fds, NULL, NULL, &timeout) > 0) {
          if (read(fd, buff, 20) <= 0) {
               break;
          }
     }
}

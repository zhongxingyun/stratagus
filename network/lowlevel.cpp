//   ___________		     _________		      _____  __
//   \_	  _____/______   ____   ____ \_   ___ \____________ _/ ____\/  |_
//    |    __) \_  __ \_/ __ \_/ __ \/    \  \/\_  __ \__  \\   __\\   __\ 
//    |     \   |  | \/\  ___/\  ___/\     \____|  | \// __ \|  |   |  |
//    \___  /   |__|    \___  >\___  >\______  /|__|  (____  /__|   |__|
//	  \/		    \/	   \/	     \/		   \/
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name lowlevel.c	-	The network lowlevel. */
//
//	(c) Copyright 2000,2001 by Lutz Sammer
//
//	$Id$

// FIXME: TCP support missing (not needed currently for freecraft)

//@{

//----------------------------------------------------------------------------
//	Includes
//----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "freecraft.h"
#include "net_lowlevel.h"
#include "etlib/dllist.h"
#include "network.h"

//----------------------------------------------------------------------------
//	Declarations
//----------------------------------------------------------------------------

#define MAX_LOC_IP 10

//----------------------------------------------------------------------------
//	Variables
//----------------------------------------------------------------------------

global unsigned long NetLastHost;	/// Last host number (net format)
global int NetLastPort;			/// Last port number (net format)

global unsigned long NetLocalAddrs[MAX_LOC_IP]; /// Local IP-Addrs of this host (net format)

//----------------------------------------------------------------------------
//	Low level functions
//----------------------------------------------------------------------------

/**@name lowlevel */
//@{

#ifdef USE_SDL_NET	// {

// FIXME: Not written, I (johns) leave this for other people.
//	  ARI: Really? Seems to exist..

/**
**	Hardware dependend network init.
*/
global int NetInit(void)
{
    return SDLNet_Init();
}

/**
**	Hardware dependend network exit.
*/
global void NetExit(void)
{
    SDLNet_Exit();
}

/**
**	Close an UDP socket port.
**
**	@param sockfd	Socket fildes;
*/
global void NetCloseUDP(sock)
{
    SDLNet_UDP_Close(UDPsocket sock);
}

#endif	// } USE_SDL_NET

#ifdef USE_WINSOCK	// {

/**
**	Hardware dependend network init.
*/
global int NetInit(void)
{
    WSADATA wsaData;

    // Start up the windows networking
    if ( WSAStartup(MAKEWORD(1,1), &wsaData) ) {
	fprintf(stderr,"Couldn't initialize Winsock 1.1\n");
	return -1;
    }
    return 0;
}

/**
**	Hardware dependend network exit.
*/
global void NetExit(void)
{
    // Clean up windows networking
    if ( WSACleanup() == SOCKET_ERROR ) {
	if ( WSAGetLastError() == WSAEINPROGRESS ) {
	    WSACancelBlockingCall();
	    WSACleanup();
	}
    }
}

/**
**	Close an UDP socket port.
**
**	@param sockfd	Socket fildes;
*/
global void NetCloseUDP(int sockfd)
{
    closesocket(sockfd);
}

#endif	// } !USE_WINSOCK

#if !defined(USE_SDL_NET) && !defined(USE_WINSOCK)	// {

/**
**	Hardware dependend network init.
*/
global int NetInit(void)
{
    return 0;
}

/**
**	Hardware dependend network exit.
*/
global void NetExit(void)
{
}

/**
**	Close an UDP socket port.
**
**	@param sockfd	Socket fildes;
*/
global void NetCloseUDP(int sockfd)
{
    close(sockfd);
}

#endif	// } !USE_SDL_NET && !USE_WINSOCK

/**
**	Resolve host in name or dotted quad notation.
**
**	@param host	Host name.
*/
global unsigned long NetResolveHost(const char* host)
{
    unsigned long addr;

    if( host ) {
	addr=inet_addr(host);		// try dot notation
	if( addr==INADDR_NONE ) {
	    struct hostent *he;

	    he=gethostbyname(host);
	    if( he ) {
		addr=0;
		DebugCheck( he->h_length!=4 );
		memcpy(&addr,he->h_addr,he->h_length);
	    }
	}
	return addr;
    }
    return INADDR_NONE;
}

#ifdef NEW_NETMENUS
// ARI: I knew how to write this for a unix environment,
//	but am quite certain that porting this can cause you
//	trouble, esp. to win32. - winipcfg can do, so it's possible..
/**
**	Get IP-addrs of local interfaces from Network file descriptor
**	and store them in the NetLocalAddrs array.
**
**	@param sock	local socket.
**
**	@returns	number of IP-addrs found.
*/
global int NetSocketAddr(const int sock)
{
    char buf[4096], *cp, *cplim;
    struct ifconf ifc;
    struct ifreq ifreq, *ifr;
    struct sockaddr_in *sap, sa;
    int i, nif;
	
    nif = 0;
    if (sock != -1) {
	ifc.ifc_len = sizeof (buf);
	ifc.ifc_buf = buf;
	if (ioctl(sock, SIOCGIFCONF, (char *)&ifc) < 0) {
	    DebugLevel0Fn("SIOCGIFCONF - errno %d\n", errno);
	    return 0;
	}
	/* with some inspiration from routed.. */
        ifr = ifc.ifc_req;
	cplim = buf + ifc.ifc_len; /*skip over if's with big ifr_addr's */
	for (cp = buf; cp < cplim; cp += sizeof (ifr->ifr_name) + sizeof(ifr->ifr_ifru)) {
	    ifr = (struct ifreq *)cp;
	    ifreq = *ifr;
	    if (ioctl(sock, SIOCGIFFLAGS, (char *)&ifreq) < 0) {
		DebugLevel0Fn("%s: SIOCGIFFLAGS - errno %d\n", ifr->ifr_name, errno);
		continue;
	    }
	    if ((ifreq.ifr_flags & IFF_UP) == 0 || ifr->ifr_addr.sa_family == AF_UNSPEC) {
		continue;
	    }
	    /* argh, this'll have to change sometime */
	    if (ifr->ifr_addr.sa_family != AF_INET) {
		continue;
	    }
	    if (ifreq.ifr_flags & IFF_LOOPBACK) {
		continue;
	    }
	    sap = (struct sockaddr_in *)&ifr->ifr_addr;
	    sa = *sap;
	    NetLocalAddrs[nif] = sap->sin_addr.s_addr;
	    if (ifreq.ifr_flags & IFF_POINTOPOINT) {
		if (ioctl(sock, SIOCGIFDSTADDR, (char *)&ifreq) < 0) {
		    DebugLevel0Fn("%s: SIOCGIFDSTADDR - errno %d\n", ifr->ifr_name, errno);
		    /* failed to obtain dst addr - ignore */
		    continue;
		}
		if (ifr->ifr_addr.sa_family == AF_UNSPEC) {
		    continue;
		}
	    }
	    /* avoid p-t-p links with common src */
	    if (nif) {
		for (i = 0; i < nif; i++) {
		    if (sa.sin_addr.s_addr == NetLocalAddrs[i]) {
			i = -1;
			break;
		    }
		}
		if (i == -1)
		    continue;
	    }
	    DebugLevel3Fn("FOUND INTERFACE %s: %d.%d.%d.%d\n", ifr->ifr_name, 
			NIPQUAD(ntohl(NetLocalAddrs[nif])));
	    nif++;
	    if (nif == MAX_LOC_IP)
		break;
	}
    }
    return nif;
}
#endif

/**
**	Open an UDP Socket port.
**
**	@param port	!=0 Port to bind in host notation.
**
**	@return		If success the socket fildes, -1 otherwise.
*/
global int NetOpenUDP(int port)
{
    int sockfd;

    // open the socket
    sockfd=socket(AF_INET, SOCK_DGRAM, 0);
    DebugLevel3Fn(" socket %d\n",sockfd);
    if( sockfd==INVALID_SOCKET ) {
	return -1;
    }
    // bind local port
    if( port ) {
	struct sockaddr_in sock_addr;

	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = INADDR_ANY;
	sock_addr.sin_port = htons(port);
	// Bind the socket for listening
	if ( bind(sockfd,(struct sockaddr*)&sock_addr,sizeof(sock_addr))<0 ) {
	    fprintf(stderr,"Couldn't bind to local port\n");
	    NetCloseUDP(sockfd);
	    return -1;
	}
	DebugLevel3Fn(" bind ok\n");
	NetLastHost=sock_addr.sin_addr.s_addr;
	NetLastPort=sock_addr.sin_port;
    }
    return sockfd;
}

/**
**	Wait for socket ready.
**
**	@param sockfd	Socket fildes to prove.
**	@param timeout	Timeout in 1/1000 seconds.
**
**	@return		1 if data is available, 0 if not, -1 if failure.
*/
global int NetSocketReady(int sockfd,int timeout)
{
    int retval;
    struct timeval tv;
    fd_set mask;

    //	Check the file descriptors for available data
    do {
	// Set up the mask of file descriptors
	FD_ZERO(&mask);
	FD_SET(sockfd, &mask);

	// Set up the timeout
	tv.tv_sec = timeout/1000;
	tv.tv_usec = (timeout%1000)*1000;

	// Data available?
	retval = select(sockfd+1, &mask, NULL, NULL, &tv);
    } while ( retval==-1 && errno == EINTR );

    return retval;
}

/**
**	Receive from an UDP socket.
*/
global int NetRecvUDP(int sockfd,void* buf,int len)
{
    int n;
    struct sockaddr_in sock_addr;

    n=sizeof(struct sockaddr_in);
    if( recvfrom(sockfd,buf,len,0,(struct sockaddr*)&sock_addr,&n)<0 ) {
	fprintf(stderr,__FUNCTION__": Could not read from UDP socket\n");
	return 0;
    }
    // FIXME: ARI: verify that it _really_ is from one of our hosts...
    // imagine what happens when an udp port scan hits the port...
    NetLastHost=sock_addr.sin_addr.s_addr;
    NetLastPort=sock_addr.sin_port;
    DebugLevel3Fn(" %d.%d.%d.%d:%d\n",
	    NIPQUAD(ntohl(NetLastHost)),ntohs(NetLastPort));
    return 1;
}

/**
**	Send through an UPD socket to a host:port.
*/
global int NetSendUDP(int sockfd,unsigned long host,int port
	,const void* buf,int len)
{
    int n;
    struct sockaddr_in sock_addr;

    n=sizeof(struct sockaddr_in);
    sock_addr.sin_addr.s_addr = host;
    sock_addr.sin_port = port;
    sock_addr.sin_family = AF_INET;

    //if( MyRand()%2 ) return 0;

    return sendto(sockfd,buf,len,0,(struct sockaddr*)&sock_addr,n);
}

//@}

//@}

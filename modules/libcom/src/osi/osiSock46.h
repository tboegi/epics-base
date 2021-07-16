/************************************************************************* \
* SPDX-License-Identifier: EPICS
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/*
 *      socket support library API def IPv4 IPv6
 *
 */
#ifndef osiSock46h
#define osiSock46h

#include "libComAPI.h"
#include "osdSock.h"
#include "ellLib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EPICSSOCKET_PRINT_DEBUG_FLAG       (1<<0)
#define EPICSSOCKETENABLEADDRESSREUSE_FLAG (1<<1)
#define EPICSSOCKETENABLEBROADCASTS_FLAG   (1<<2)
#define EPICSSOCKETBINDLOCALPORT_FLAG      (1<<3)
#define EPICSSOCKET_CONNECT_IPV4           (1<<4)
/* leave out 5 */
#define EPICSSOCKET_CONNECT_IPV6           (1<<6)


struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;
struct in_addr;

typedef union osiSockAddr46 {
    struct sockaddr     sa;
    struct sockaddr_in  in; /* We call it "in", not ia */
    struct sockaddr_in6 in6 /* in6 for IPv6 */;
} osiSockAddr46;

LIBCOM_API SOCKET epicsStdCall epicsSocketCreate (
    int domain, int type, int protocol );

LIBCOM_API unsigned epicsStdCall ipAddrToDottedIP64 (
    const osiSockAddr46 *paddr, char *pBuf, unsigned bufSize
                                                            );
LIBCOM_API SOCKET epicsStdCall epicsSocketCreateBind46 (
    int domain, int type, int protocol,
    unsigned short     localPort,
    int                flags,
    char               *pErrorMessage,
    size_t             errorMessageSize
                                                        );

LIBCOM_API SOCKET epicsStdCall epicsSocketCreateBindConnect46 (
    int domain, int type, int protocol,
    const char         *hostname,
    unsigned short     port,
    unsigned short     localPort,
    int                flags,
    char               *pErrorMessage,
    size_t             errorMessageSize);


LIBCOM_API int epicsStdCall epicsSocketAccept46 (
    int sock, struct sockaddr * pAddr, osiSocklen_t * addrlen );

/* this belongs to osiSock.[ch] */
LIBCOM_API void epicsStdCall epicsSocketDestroy (
    SOCKET );
/*
 * attach to BSD socket library
 */
LIBCOM_API int epicsStdCall osiSockAttach (void); /* returns T if success, else F */

/*
 * convert socket address to ASCII in this order
 * 1) look for matching host name and typically add trailing IP port
 * 2) failing that, convert to raw ascii address (typically this is a
 *      dotted IP address with trailing port)
 * 3) failing that, writes "<Ukn Addr Type>" into pBuf
 *
 * returns the number of character elements stored in buffer not
 * including the null termination, but always writes at least a
 * null ternminater in the string (if bufSize >= 1)
 */
LIBCOM_API unsigned epicsStdCall sockAddrToA46 (
    const struct sockaddr * paddr, char * pBuf, unsigned bufSize );

/*
 * convert IP address to ASCII in this order
 * 1) look for matching host name and add trailing port
 * 2) convert to raw dotted IP address with trailing port
 *
 * returns the number of character elements stored in buffer not
 * including the null termination, but always writes at least a
 * null ternminater in the string (if bufSize >= 1)
 */
LIBCOM_API unsigned epicsStdCall ipAddrToA46 (
    const struct sockaddr_in * pInetAddr, char * pBuf, unsigned bufSize );

/*
 * sockAddrToDottedIP ()
 * typically convert to raw dotted IP address with trailing port
 *
 * returns the number of character elements stored in buffer not
 * including the null termination, but always writes at least a
 * null ternminater in the string (if bufSize >= 1)
 */
LIBCOM_API unsigned epicsStdCall sockAddrToDottedIP46 (
    const struct sockaddr * paddr, char * pBuf, unsigned bufSize );

/*
 * ipAddrToDottedIP ()
 * convert to raw dotted IP address with trailing port
 *
 * returns the number of character elements stored in buffer not
 * including the null termination, but always writes at least a
 * null ternminater in the string (if bufSize >= 1)
 */
LIBCOM_API unsigned epicsStdCall ipAddrToDottedIP46 (
    const struct sockaddr_in * paddr, char * pBuf, unsigned bufSize );

/*
 * convert inet address to a host name string
 *
 * returns the number of character elements stored in buffer not
 * including the null termination. This will be zero if a matching
 * host name cant be found.
 *
 * there are many OS specific implementation stubs for this routine
 */
LIBCOM_API unsigned epicsStdCall ipAddrToHostName46 (
    const struct in_addr * pAddr, char * pBuf, unsigned bufSize );

/*
 * attempt to convert ASCII string to an IP address in this order
 * 1) look for traditional doted ip with optional port
 * 2) look for raw number form of ip address with optional port
 * 3) look for valid host name with optional port
 */
LIBCOM_API int epicsStdCall aToIPAddr46
    ( const char * pAddrString, unsigned short defaultPort, struct sockaddr_in * pIP);

/*
 * attempt to convert ASCII host name string with optional port to an IP address
 */
LIBCOM_API int epicsStdCall hostToIPAddr46
    (const char *pHostName, struct in_addr *pIPA);

#ifdef __cplusplus
}
#endif

#endif /* ifndef osiSock46h */

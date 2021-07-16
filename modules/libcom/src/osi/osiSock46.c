/************************************************************************* \
* SPDX-License-Identifier: EPICS
* EPICS Base is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/*
 *      socket support library generic code for IPv4 and IPv6
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <epicsString.h>
#include "epicsAssert.h"
#include "epicsSignal.h"
#include "epicsStdio.h"
#include "osiSock46.h"

#ifndef SO_REUSEPORT
# define USE_SO_REUSEADDR
#endif

#define errlogPrintf printf

LIBCOM_API SOCKET epicsStdCall epicsSocketCreateBind46(
    int domain, int type, int protocol,
    unsigned short     localPort,
    int                flags,
    char               *pErrorMessage,
    size_t             errorMessageSize
                                                               )
{
  if (flags & EPICSSOCKET_PRINT_DEBUG_FLAG) {
    errlogPrintf("osiSock46: epicsSocketCreateBind46 domain/family=%d type=%d protocol=%d localPort=%u flags=0x%x\n",
                 domain, type, protocol, localPort, flags);
  }
  SOCKET fd = epicsSocketCreate(domain, type, protocol);

  if (fd  == INVALID_SOCKET) {
    if (pErrorMessage) {
      epicsSnprintf(pErrorMessage, errorMessageSize,
                    "Can't create socket: %s", strerror(SOCKERRNO));
    }
    if (flags & EPICSSOCKET_PRINT_DEBUG_FLAG) {
      errlogPrintf("osiSock46: Can't create socket: %s\n", strerror(SOCKERRNO));
    }
    return INVALID_SOCKET;
  }

  if (flags & EPICSSOCKETENABLEBROADCASTS_FLAG) {
    /*
     * Enable broadcasts if so requested
     */
    int i = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (void *)&i, sizeof i) < 0) {
      if (pErrorMessage) {
        epicsSnprintf(pErrorMessage,errorMessageSize,
                      "Can't set socket BROADCAST option: %s",
                      strerror(SOCKERRNO));
      }
      if (flags & EPICSSOCKET_PRINT_DEBUG_FLAG) {
        errlogPrintf("osiSock46: Can't set socket BROADCAST option:  %s\n",
                     strerror(SOCKERRNO));
      }
      epicsSocketDestroy(fd);
      return INVALID_SOCKET;
    }
  }
  if (flags & EPICSSOCKETENABLEADDRESSREUSE_FLAG) {
    /*
     * Enable SO_REUSEPORT if so requested
     */
    int i = 1;
#ifdef USE_SO_REUSEADDR
    int sockOpt = SO_REUSEADDR;
#else
    int sockOpt = SO_REUSEPORT;
#endif
    if (setsockopt(fd, SOL_SOCKET, sockOpt, (void *)&i, sizeof i) < 0) {
      if (pErrorMessage) {
        epicsSnprintf(pErrorMessage, errorMessageSize,
                      "Can't set socket SO_REUSEPORT option: %s",
                      strerror(SOCKERRNO));
      }
      if (flags & EPICSSOCKET_PRINT_DEBUG_FLAG) {
        errlogPrintf("osiSock46: Can't set socket SO_REUSEPORT option:  %s\n",
                      strerror(SOCKERRNO));
      }
      epicsSocketDestroy(fd);
      return INVALID_SOCKET;
    }
  }

  if (flags & EPICSSOCKETBINDLOCALPORT_FLAG)  {
    osiSockAddr46 addr;
    int status = -2;
    memset(&addr, 0 , sizeof(addr));
    if (domain == AF_INET) {
      addr.in.sin_family = domain;
      addr.in.sin_addr.s_addr = htonl (INADDR_ANY);
      addr.in.sin_port = htons (localPort);
      status = bind(fd, &addr.sa, sizeof(addr.in));
    } else if (domain == AF_INET6) {
      addr.in6.sin6_family = domain;
      addr.in6.sin6_port = htons(localPort);
       /* Address of sa, length of in6: */
      status = bind(fd, &addr.sa, sizeof(addr.in6));
    }
    if (status) {
      if (pErrorMessage) {
        epicsSnprintf(pErrorMessage, errorMessageSize,
                      "Can not bind: status=%d %s\n",
                      status, strerror(SOCKERRNO));
      }
      if (flags & EPICSSOCKET_PRINT_DEBUG_FLAG) {
        errlogPrintf("osiSock46: Can't bind status=%d: %s\n",
                     status, strerror(SOCKERRNO));
      }
      epicsSocketDestroy(fd);
      return INVALID_SOCKET;
    }
  }
  return fd;
}

LIBCOM_API SOCKET epicsStdCall epicsSocketCreateBindConnect46(
    int domain, int type, int protocol,
    const char         *hostname,
    unsigned short     port,
    unsigned short     localPort,
    int                flags,
    char               *pErrorMessage,
    size_t             errorMessageSize)
{
  struct addrinfo *ai, *ai_to_free;
  struct addrinfo hints;
  int gai;
  SOCKET fd = INVALID_SOCKET;
  char tmpErrorMessage[40];
  char service_ascii [8];
  char *hostname_to_delete = NULL;

  if (flags & EPICSSOCKET_PRINT_DEBUG_FLAG) {
    errlogPrintf("osiSock46: epicsSocketCreateBindConnect46 domain/family=%d type=%d protocol=%d hostname=%s port=%u flags=0x%x\n",
                 domain, type, protocol,
                 hostname ? hostname : 0,
                 port,
                 flags);
  }
  if (!hostname) {
    return epicsSocketCreateBind46(domain, type, protocol, localPort, flags,
                                   pErrorMessage, errorMessageSize);
  }
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = type;
  if (domain == AF_INET6) {
    /* we could find both IPv4 and Ipv6 addresses */
    if (flags & EPICSSOCKET_CONNECT_IPV4)
      hints.ai_family = AF_INET;
    else if (flags & EPICSSOCKET_CONNECT_IPV6)
      hints.ai_family = AF_INET6;
  } else {
    hints.ai_family = domain;
  }
  /* We are tolerant and accept [hostname].
     This allows to specify [example.com] to use IPv6 only */
  if (hostname[0] == '[') {
    char *cp;
    hostname_to_delete = epicsStrDup(&hostname[1]);
    cp = strchr(hostname_to_delete, ']');
    if (cp) *cp = '\0';
    hints.ai_family = AF_INET6;
  } else if (hostname[0] == ']') {
    hostname_to_delete = epicsStrDup(&hostname[1]);
    hints.ai_family = AF_INET;
  }


  /* 2 options here: supply the port number as ASCII
     or patch the resulting ai struct later */
  epicsSnprintf(service_ascii, sizeof(service_ascii), "%u", port);
  gai = getaddrinfo(hostname_to_delete ? hostname_to_delete : hostname,
                    service_ascii, &hints, &ai);
  if (gai) {
    if (pErrorMessage) {
      epicsSnprintf(pErrorMessage, errorMessageSize,
                    "unable to look up hostname %s %s (%s)",
                    hostname,
                    hostname_to_delete ? hostname_to_delete : "",
                    gai_strerror(gai));
    }
    if (flags & EPICSSOCKET_PRINT_DEBUG_FLAG) {
      errlogPrintf("osiSock46: unable to look up hostname %s %s (%s)\n",
                   hostname,
                   hostname_to_delete ? hostname_to_delete : "",
                   gai_strerror(gai));
    }
    free(hostname_to_delete);
    return INVALID_SOCKET;
  }
  free(hostname_to_delete);
  hostname_to_delete = NULL;
  /* now loop through the addresses */
  for (ai_to_free = ai; ai; ai = ai->ai_next) {
    memset(tmpErrorMessage, 0, sizeof(tmpErrorMessage));
    fd = epicsSocketCreateBind46(ai->ai_family, ai->ai_socktype, ai->ai_protocol,
                                 localPort, flags,
                                 tmpErrorMessage, sizeof(tmpErrorMessage));
    if (fd != INVALID_SOCKET) {
      int status;
      osiSocklen_t socklen = ai->ai_addrlen;
      if (ai->ai_family == AF_INET) socklen = (osiSocklen_t) sizeof(struct sockaddr_in);
      status = connect(fd, ai->ai_addr, ai->ai_addrlen);
      if (flags & EPICSSOCKET_PRINT_DEBUG_FLAG) {
        char ipHostASCII[64];
        memset(ipHostASCII, 0, sizeof(ipHostASCII));
        ipAddrToDottedIP64((osiSockAddr46 *)ai->ai_addr,
                           ipHostASCII,
                           sizeof(ipHostASCII) - 1);
        errlogPrintf("osiSock46: connect to hostname %s (%s) domain/family=%d addrlen=%u socklen=%u status=%d (%s)\n",
                     hostname, ipHostASCII,
                     ai->ai_family, (unsigned)ai->ai_addrlen,
                     (unsigned)socklen, status,
                     status ? strerror(SOCKERRNO) : "");
        if (!status) {
          return fd;
        } else {
          epicsSocketDestroy(fd);
          fd = INVALID_SOCKET;
        }
      }
    }
  }
  /*  If things had gone wrong
   *  retrieve the last error message
   */
  if (fd == INVALID_SOCKET && pErrorMessage) {
    epicsSnprintf(pErrorMessage, errorMessageSize,
                  "%s", tmpErrorMessage);
  }
  freeaddrinfo(ai_to_free);
  return fd;
}


LIBCOM_API unsigned epicsStdCall ipAddrToDottedIP64(
    const osiSockAddr46 *paddr, char *pBuf, unsigned bufSize)
{
  static const char *pErrStr = "[snip]";
  unsigned strLen;
  int status = -1;

  if (!bufSize) return 0;

  if (paddr->in.sin_family == AF_INET) {
    unsigned addr = ntohl(paddr->in.sin_addr.s_addr);
      /*
       * inet_ntoa() isnt used because it isnt thread safe
       * (and the replacements are not standardized)
       */
      status = epicsSnprintf(pBuf, bufSize, "%u.%u.%u.%u:%hu",
                             (addr >> 24) & 0xFF,
                             (addr >> 16) & 0xFF,
                             (addr >> 8) & 0xFF,
                             (addr) & 0xFF,
                             ntohs ( paddr->in.sin_port));
  } else if (paddr->in6.sin6_family == AF_INET6) {
    if (0 && IN6_IS_ADDR_V4MAPPED(&paddr->in6.sin6_addr)) {
      status = epicsSnprintf(pBuf, bufSize, "%u.%u.%u.%u:%hu",
                             paddr->in6.sin6_addr.s6_addr[12],
                             paddr->in6.sin6_addr.s6_addr[13],
                             paddr->in6.sin6_addr.s6_addr[14],
                             paddr->in6.sin6_addr.s6_addr[15],
                             ntohs ( paddr->in6.sin6_port));
    } else {
      status = epicsSnprintf (pBuf, bufSize,
                              "[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%hu",
                              paddr->in6.sin6_addr.s6_addr[0],
                              paddr->in6.sin6_addr.s6_addr[1],
                              paddr->in6.sin6_addr.s6_addr[2],
                              paddr->in6.sin6_addr.s6_addr[3],
                              paddr->in6.sin6_addr.s6_addr[4],
                              paddr->in6.sin6_addr.s6_addr[5],
                              paddr->in6.sin6_addr.s6_addr[6],
                              paddr->in6.sin6_addr.s6_addr[7],
                              paddr->in6.sin6_addr.s6_addr[8],
                              paddr->in6.sin6_addr.s6_addr[9],
                              paddr->in6.sin6_addr.s6_addr[10],
                              paddr->in6.sin6_addr.s6_addr[11],
                              paddr->in6.sin6_addr.s6_addr[12],
                              paddr->in6.sin6_addr.s6_addr[13],
                              paddr->in6.sin6_addr.s6_addr[14],
                              paddr->in6.sin6_addr.s6_addr[15],
                              ntohs ( paddr->in6.sin6_port));
    }
  } else {
    status = epicsSnprintf(pBuf, bufSize, "invfam=%u",
                           paddr->in.sin_family);
  }
  if (status > 0) {
    strLen = (unsigned) status;
    if (strLen < bufSize - 1) {
      return strLen;
    }
  }
  strLen = strlen (pErrStr);
  if (strLen < bufSize) {
    strcpy (pBuf, pErrStr);
    return strLen;
  } else {
    strncpy ( pBuf, pErrStr, bufSize);
    pBuf[bufSize-1] = '\0';
    return bufSize - 1u;
  }
}

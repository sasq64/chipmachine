/*
    NetLink Sockets: Networking C++ library
    Copyright 2012 Pedro Francisco Pareja Ruiz (PedroPareja@Gmail.com)

    This file is part of NetLink Sockets.

    NetLink Sockets is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    NetLink Sockets is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with NetLink Sockets. If not, see <http://www.gnu.org/licenses/>.

*/

/**
* @file core.h
* NetLink core types and init
*/


#ifndef __NL_CORE
#define __NL_CORE

#include "netlink/config.h"

#define NL_NAMESPACE_NAME NL

#define NL_NAMESPACE namespace NL_NAMESPACE_NAME {
#define NL_NAMESPACE_END };
#define NL_NAMESPACE_USE using namespace NL_NAMESPACE_NAME;


#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)

    #define OS_WIN32
    //#define _WIN32_WINNT 0x0600


    #include <winsock2.h>
    #include <ws2tcpip.h>

    // Requires Win7 or Vista
    // Link to Ws2_32.lib library


	#define snprintf _snprintf

#else

    #define OS_LINUX

    #include <arpa/inet.h>
    #include <sys/fcntl.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/unistd.h>
    #include <sys/time.h>
    #include <netdb.h>
    #include <sys/ioctl.h>
    #include <errno.h>

#endif

#include <string>


NL_NAMESPACE

using std::string;


void init();


/**
* @enum Protocol
*
* Defines protocol type.
*/

enum Protocol {

    TCP,
    UDP
};


/**
* @enum IPVer
*
* Defines the version of IP.
*/

enum IPVer {

    IP4,
    IP6,
    ANY
};


/**
* @enum SocketType
*
* Defines the nature of the socket.
*/

enum SocketType {

    CLIENT,     /**< TCP or UDP socket connected or directed to a target host*/
    SERVER      /**< TCP socket which listens for connections or UDP socket without target host*/
};

NL_NAMESPACE_END


#include "netlink/exception.h"
#include "netlink/release_manager.h"
#include "netlink/util.h"

#endif

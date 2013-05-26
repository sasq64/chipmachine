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

#ifndef __NET_LINK
#define __NET_LINK


/**
* @example clientEcho.cc
* Example of TCP CLIENT Socket
*
* @example serverEcho.cc
* Example of TCP SERVER Socket
*
* @example udpDirectChat.cc
* Example of UDP non-blocking Socket
*
* @example chatClient.cc
* Example of SocketGroup
*
* @example chatServer.cc
* Example of SocketGroup
*
* @example webGet.cc
* Example of SmartBuffer and SocketGroup.
* Retrieves a web page and prints it.
*/

/**
* @namespace NL
* NetLink Sockets Namespace
*/


/**
* @mainpage NetLink Sockets C++ Library
* @author Pedro Fco. Pareja Ruiz ( PedroPareja [at] Gmail.com )
* @version 1.0.0-pre-4
*
* This is a C++ socket library designed to enable easy and fast development of socket related functionality.
*
* @warning Do not forget to call NL::init() in first place for the library initialization. This is
* only necessary in windows, but in the others OS will not do any harm.
*
* @note Since 1.0.0, NetLink Sockets C++ can be used in Windows XP (earlier versions require at least Windows Vista to be used in Windows OS)
*
* All the components of NetLink Sockets are in NL namespace.
*
* Download the latest version of the library at http://sourceforge.net/projects/netlinksockets/
*
* @section CHANGELOG
*
* 1.0.0-pre-4
* @li Fixed: memory leak in NL::Socket::initSocket(): some blocks of addrinfo were not completely freed
*
* 1.0.0-pre-3
* @li Fixed example: NL::init() was missing in udpDirectChat.cc
* @li Added sourceForge logo to documentation
*
* 1.0.0-pre-2
* @li Added SmartBuffer Class
* @li Added webGet.cc example
* @li Documentation improvements
*
* 1.0.0-pre-1
* @li First v1 release
*/



#include <netlink/socket.h>
#include <netlink/socket_group.h>


#endif

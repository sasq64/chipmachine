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

#ifdef DOXYGEN
    #include <netlink/socket.h>
    NL_NAMESPACE
#endif

/**
* Returns the target host of the socket
*
* @return the host this socket is connected to (in TCP
* case) or the host it sends data to (UDP case)
*/

inline const string& Socket::hostTo() const {

    return _hostTo;
}

/**
* Returns the socket local address
*
* @return socket local address
*/

inline const string& Socket::hostFrom() const {

    return _hostFrom;
}

/**
* Returns the port this socket is connected/sends to
*
* @return target/remote port
*/

inline unsigned Socket::portTo() const {

    return _portTo;
}

/**
* Returns the local port of the socket
*
* @return local port
*/

inline unsigned Socket::portFrom() const {

    return _portFrom;
}

/**
* Returns the socket protocol
* @return Protocol
*/

inline Protocol Socket::protocol() const {

    return _protocol;
}

/**
* Returns the socket IP version
* @return IPVer
*/

inline IPVer Socket::ipVer() const {

    return _ipVer;
}

/**
* Returns the socket type
* @return SocketType (SERVER or CLIENT)
*/

inline SocketType Socket::type() const {

    return _type;
}

/**
* Returns the size of the listen queue.
*
* @pre The Socket must be SERVER, otherwise this call has no sense and returns 0
* @return the size of the internal buffer of the SERVER TCP socket where the connection
*   requests are stored until accepted
*/

inline unsigned Socket::listenQueue() const {

    return _listenQueue;
}

/**
* Returns whether the socket is blocking (true) or not (false)
*
* @return socket blocking status
*/

inline bool Socket::blocking() const {

    return _blocking;
}


/**
* Returns the socket handler (file/socket descriptor)
*
* @return File descritor to handle the socket
*/


inline int Socket::socketHandler() const {

    return _socketHandler;
}

#ifdef DOXYGEN
    NL_NAMESPACE_END
#endif



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
    #include <netlink/socket_group.h>
    NL_NAMESPACE
#endif


/**
* Adds the Socket to the SocketGroup
*
* @param socket Socket to be added
*/


inline void SocketGroup::add(Socket* socket) {

    _vSocket.push_back(socket);
}

/**
* Gets the pointer to the Socket of position index in the SocketGroup
*
* @param index Socket position
* @return A pointer to the Socket
*
* @throw Exception OUT_OF_RANGE
*/

inline Socket* SocketGroup::get(unsigned index) const {

    if(index >= _vSocket.size())
        throw Exception(Exception::OUT_OF_RANGE, "SocketGroup::get: index out of range");

    return _vSocket[index];
}

/**
* Removes from the group the Socket of position index
*
* @param index Socket position
*
* @throw Exception OUT_OF_RANGE
*/

inline void SocketGroup::remove(unsigned index) {

    if(index >= _vSocket.size())
        throw Exception(Exception::OUT_OF_RANGE, "SocketGroup::remove: index out of range");

    _vSocket.erase(_vSocket.begin() + index);
}

/**
* Returns the size of the group
*
* @return The number of Sockets contained in the SocketGroup
*/

inline size_t SocketGroup::size() const {

    return _vSocket.size();
}

/**
* Sets the onAcceptReady callback
*
* This callback will be call for any incoming connection waiting to be accepted
* @param cmd SocketGroupCmd implementing the desired callback in exec() function
* @warning Only used for TCP protocol SERVER sockets
*/

inline void SocketGroup::setCmdOnAccept(SocketGroupCmd* cmd) {

    _cmdOnAccept = cmd;
}

/**
* Sets the onReadReady callback
*
* This callback will be call for any incoming data waiting to be read
* @param cmd SocketGroupCmd implementing the desired callback in exec() function
*/


inline void SocketGroup::setCmdOnRead(SocketGroupCmd* cmd) {

    _cmdOnRead = cmd;
}

/**
* Sets the onDisconnect callback
*
* This callback will be call for any disconnection detected
* @param cmd SocketGroupCmd implementing the desired callback in exec() function
* @warning Only used for TCP protocol CLIENT sockets
*/


inline void SocketGroup::setCmdOnDisconnect(SocketGroupCmd* cmd) {

    _cmdOnDisconnect = cmd;
}

#ifdef DOXYGEN
    NL_NAMESPACE_END
#endif


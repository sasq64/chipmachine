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

#ifndef __NL_SOCKET_GROUP
#define __NL_SOCKET_GROUP

#include <netlink/core.h>
#include <netlink/socket.h>

NL_NAMESPACE

using std::vector;


class SocketGroup;

/**
* @class SocketGroupCmd socket_group.h netlink/socket_group.h
*
* Class to be used as base for SocketGroup callback function implementing classes
*/

class SocketGroupCmd {

    public:

        /**
        * Function to be implemented for the callback. The parameters will be provided by the
        * SocketGroup::listen() function when calling
        *
        * @param socket Socket which triggered the callback
        * @param group SocketGroup which triggered the callback
        * @param reference Pointer passed to listen function to be used here
        */

        virtual void exec(Socket* socket, SocketGroup* group, void* reference)=0;
};


/**
* @class SocketGroup socket_group.h netlink/socket_group.h
*
* To manage sockets and connections
*/

class SocketGroup {

    private:

        vector<Socket*> _vSocket;

        SocketGroupCmd* _cmdOnAccept;
        SocketGroupCmd* _cmdOnRead;
        SocketGroupCmd* _cmdOnDisconnect;

    public:

        SocketGroup();

        void add(Socket* socket);
        Socket* get(unsigned index) const;
        void remove(unsigned index);
        void remove(Socket* socket);

        size_t size() const;

        void setCmdOnAccept(SocketGroupCmd* cmd);
        void setCmdOnRead(SocketGroupCmd* cmd);
        void setCmdOnDisconnect(SocketGroupCmd* cmd);

        bool listen(unsigned milisec=0, void* reference = NULL);
};

#include <netlink/socket_group.inline.h>

NL_NAMESPACE_END

#endif

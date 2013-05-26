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

#include <netlink/socket_group.h>

NL_NAMESPACE_USE


; // <-- this is for doxygen not to get confused by NL_NAMESPACE_USE
/**
* SocketGroup constructor
*/

SocketGroup::SocketGroup(): _cmdOnAccept(NULL), _cmdOnRead(NULL), _cmdOnDisconnect(NULL) {}


/**
* Removes a socket of the group
*
* @param socket The socket we want to be removed
* @warning This removes only one reference to the socket: if the socket was added to the group more
*  than once (you shouldn't) there will be references left
*/

void SocketGroup::remove(Socket* socket) {

    vector<Socket*>::iterator it = _vSocket.begin();

    while(it != _vSocket.end())
        if(*it == socket) {
            _vSocket.erase(it);
            return;
        }
        else
            ++it;
}


/**
* Listens for incoming data/connections
*
* Listens during milisecs time for incoming data/connections in any socket of the group calling
* the appropriate callback (Accept, Read or Disconnect) if assigned.
*
* @note UDP sockets only uses Read callback as they don not establish connections (can not accept) nor
* they register disconnections
*
* @param milisec minimum time spent listening. By defaul 0
* @param reference A pointer which can be passed to the callback functions so they have a context.
* By default NULL
* @return false if there were no incoming data, true otherwise
* @throw Exception ERROR_SELECT
*/


bool SocketGroup::listen(unsigned milisec, void* reference) {

    unsigned finTime = getTime() + milisec;
    bool executedOnce = false;
    bool result = false;

    while(getTime() < finTime || !executedOnce) {

        executedOnce = true;

        fd_set setSockets;
        int maxHandle = 0;

        FD_ZERO(&setSockets);

        for(unsigned i=0; i < _vSocket.size(); i++) {
            FD_SET(_vSocket[i]->socketHandler(), &setSockets);
            maxHandle = iMax(maxHandle, _vSocket[i]->socketHandler());
        }

        unsigned milisecLeft = finTime - getTime();
        struct timeval timeout;

        timeout.tv_sec = milisecLeft / 1000;
        timeout.tv_usec = (milisecLeft % 1000) * 1000;

        int status = select(maxHandle + 1, &setSockets, NULL, NULL, &timeout);

        if (status == -1)
            throw Exception(Exception::ERROR_SELECT, "SocketGroup::listen: could not perform socket select");

        unsigned i=0;
        int launchSockets = 0;

        while(launchSockets < status && i < _vSocket.size()) {

            if(FD_ISSET(_vSocket[i]->socketHandler(), &setSockets)) {

                launchSockets++;

                if(_vSocket[i]->type() == SERVER && _vSocket[i]->protocol() == TCP) {
                    if(_cmdOnAccept)
                        _cmdOnAccept->exec(_vSocket[i], this, reference);
                } //if
                else {
                    if(_vSocket[i]->protocol() == TCP && !_vSocket[i]->nextReadSize()) {
                        if(_cmdOnDisconnect)
                            _cmdOnDisconnect->exec(_vSocket[i], this, reference);
                    }

                    else if(_cmdOnRead)
                        _cmdOnRead->exec(_vSocket[i], this, reference);
                }

            }

            ++i;

        } //while

        if(launchSockets)
            result = true;

    } //while

    return result;

}


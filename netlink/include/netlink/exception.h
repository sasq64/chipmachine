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

#ifndef __NL_EXCEPTION
#define __NL_EXCEPTION

#include "netlink/core.h"

NL_NAMESPACE

/**
* @class Exception exception.h netlink/exception.h
*
* Exception Class
*/


class Exception {

    public:

        enum CODE {
            #include "netlink/exception.code.inc"
        };

    private:

        CODE    _code;
        string  _msg;
        int     _nativeErrorCode;

    public:

        Exception(CODE code, const string& msg, int nativeErrorCode = 0): _code(code), _msg(msg), _nativeErrorCode(nativeErrorCode) {}

        /**
        * Returns the error code:
        * @htmlinclude "exception.code.inc"
        * @return Error code
        *
        * \note Defined Error Codes:
        * \li BAD_PROTOCOL
        * \li BAD_IP_VER
        * \li ERROR_INIT
        * \li ERROR_SET_ADDR_INFO
        * \li ERROR_GET_ADDR_INFO
        * \li ERROR_SET_SOCK_OPT
        * \li ERROR_CAN_NOT_LISTEN
        * \li ERROR_CONNECT_SOCKET
        * \li ERROR_SEND
        * \li ERROR_READ
        * \li ERROR_IOCTL
        * \li ERROR_SELECT
        * \li ERROR_ALLOC
        * \li EXPECTED_TCP_SOCKET
        * \li EXPECTED_UDP_SOCKET
        * \li EXPECTED_CLIENT_SOCKET
        * \li EXPECTED_SERVER_SOCKET
        * \li EXPECTED_HOST_TO
        * \li OUT_OF_RANGE
        */

        CODE code() const           { return _code; }


        /**
        * Returns the error text message
        *
        * @return A string with a human readable error description
        */

        const string& msg() const   { return _msg; }

        /**
        * Returns the error text message
        *
        * @return A C-string with a human readable error description
        */

        const char* what() const    { return _msg.c_str(); }


        /**
        * Returns the native error code received.
        *
        * @return Native error code set by OS
        * @warning Native error codes differ depending of the OS
        */

        int nativeErrorCode() const { return _nativeErrorCode; }
};

NL_NAMESPACE_END

#endif

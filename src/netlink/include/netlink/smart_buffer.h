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


#ifndef __NL_SMART_BUFFER
#define __NL_SMART_BUFFER

#include <stdlib.h>

#include <netlink/core.h>
#include <netlink/util.h>
#include <netlink/socket.h>


NL_NAMESPACE


/**
* @class SmartBuffer smart_buffer.h netlink/smart_buffer.h
*
* Smart Buffer Class
*
* Buffer class to retrieve data of unknown size easily from a Socket
*/


class SmartBuffer {

    private:

        void*   _buffer;
        size_t  _usedSize;
        size_t  _allocSize;
        double  _reallocRatio;

    public:

        SmartBuffer(size_t allocSize = DEFAULT_SMARTBUFFER_SIZE, double reallocRatio = DEFAULT_SMARTBUFFER_REALLOC_RATIO);
        ~SmartBuffer();

        const void* operator*() const;
        const char* operator[](size_t index) const;

        const void* buffer() const;
        size_t size() const;

        void read(Socket* socket);

        void clear();
};

#include <netlink/smart_buffer.inline.h>

NL_NAMESPACE_END

#endif

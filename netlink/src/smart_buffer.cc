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


#include <netlink/smart_buffer.h>

NL_NAMESPACE_USE

;

/**
* SmartBuffer Constructor
*
* @param allocSize Size in bytes of the initial reserved memory.
* @param reallocRatio Growing ratio for memory reallocs. For example 1.5 means that
*  each time the buffer is out of memory it reserves the previous size 1.5 times (150%).
*
* @throw Exception ERROR_ALLOC
*/

SmartBuffer::SmartBuffer(size_t allocSize, double reallocRatio): _usedSize(0),
                            _allocSize(allocSize), _reallocRatio(reallocRatio)
{

    _buffer = malloc(allocSize);

    if(!_buffer)
        throw Exception(Exception::ERROR_ALLOC, "SmartBuffer::SmartBuffer: memory alloc error");

}

/**
* SmartBuffer Destructor
*/

SmartBuffer::~SmartBuffer() {

    free(_buffer);
}

/**
* Inserts the data read from a socket in the buffer
*
* @param socket Socket to be used as source
*
* @throw Exception ERROR_ALLOC, ERROR_READ*
*/


void SmartBuffer::read(Socket* socket) {

    size_t incomingBytes = socket->nextReadSize();

    do {

        size_t freeBytes = _allocSize - _usedSize;

        if(incomingBytes > freeBytes || !freeBytes) {

            size_t newAllocSize = uMax((unsigned)(_allocSize * _reallocRatio), _usedSize + incomingBytes);

            _buffer = realloc(_buffer, newAllocSize);

            if(!_buffer)
                throw Exception(Exception::ERROR_ALLOC, "SmartBuffer::Read: memory alloc error");

            _allocSize = newAllocSize;

            freeBytes = _allocSize - _usedSize;
        }

        _usedSize += socket->read((char*)_buffer + _usedSize, freeBytes);

    } while ((incomingBytes = socket->nextReadSize()));

}


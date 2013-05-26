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
    #include <netlink/smart_buffer.h>
    NL_NAMESPACE
#endif


/**
* Returns the buffer data address. Same of buffer().
*
* @return A pointer to data
*/

inline const void* SmartBuffer::operator*() const {

    return _buffer;
}


/**
* Returns the byte in position index of the buffer
*
* @param index Position of the buffer we want to retrieve
* @return Char with the content of the given position
*
* @throw Exception OUT_OF_RANGE
*/

inline const char* SmartBuffer::operator[](size_t index) const {

    if(index >= _usedSize)
        throw Exception(Exception::OUT_OF_RANGE, "SmartBuffer::operator[]: index out of range");

    return (char*) _buffer + index;
}

/**
* Returns the buffer data address. Same of operator*()
*
* @return A pointer to data
*/

inline const void* SmartBuffer::buffer() const {

    return _buffer;
}

/**
* Returns the amount of bytes of data stored in the buffer
*
* @return Size of buffer data
*/

inline size_t SmartBuffer::size() const {

    return _usedSize;
}

/**
* Clears the buffer.
*
* Resets the buffer erasing all its content.
*/

inline void SmartBuffer::clear() {

    _usedSize = 0;
}

#ifdef DOXYGEN
    NL_NAMESPACE_END
#endif


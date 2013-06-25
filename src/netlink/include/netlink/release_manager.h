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

#ifndef __NL_RM
#define __NL_RM

#include <netlink/core.h>

#include <vector>

NL_NAMESPACE

using std::vector;

/**
* @class ReleaseManager release_manager.h netlink/release_manager.h
*
* Release Manager Class
*
* Private. For internal use
*/

template<typename T>
class ReleaseManager {

    protected:

        vector<T**> _releaseQueue;
        vector<T*>  _releaseAddressQueue;

        void      (*_releaseFunction) (T*);

    public:

        ReleaseManager(void (*releaseFunction)(T*) = NULL);
        virtual ~ReleaseManager();

        void add(T** var);
        void add(T* address);
};


#include "netlink/release_manager.inline.h"

NL_NAMESPACE_END

#endif

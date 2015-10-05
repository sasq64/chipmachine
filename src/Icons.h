#ifndef ICONS_H
#define ICONS_H

#include <vector>

struct Icon {
	short w;
	short h;
	std::vector<unsigned> data;
};

#define Z 0xff444488
static const Icon heart_icon = {8,
                                6,
                                {0, Z, Z, 0, Z, Z, 0, 0, Z, Z, Z, Z, Z, Z, Z, 0, Z, Z, Z, Z, Z, Z,
                                 Z, 0, 0, Z, Z, Z, Z, Z, 0, 0, 0, 0, Z, Z, Z, 0, 0, 0, 0, 0, 0, Z,
                                 0, 0, 0, 0}};

#undef Z
#define Z 0xff44cccc
static const Icon net_icon = {8,
                              5,
                              {0, 0, Z, Z, Z, 0, 0, 0, 0, 0, Z, 0, Z, 0, 0, 0, Z, Z, Z, 0, Z, 0, Z,
                               Z, 0, 0, 0, 0, Z, 0, Z, 0, 0, 0, 0, 0, Z, Z, Z, 0}};
#undef Z

#define Z 0xffffcccc
static const Icon volume_icon = {
    19,
    19,
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, Z, 0, Z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Z, 0, Z, 0, Z, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, Z, 0, Z, 0, Z, 0, Z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Z, 0, Z, 0, Z, 0, Z,
        0, Z, 0, 0, 0, 0, 0, 0, 0, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, 0, 0, 0, 0, 0, Z, 0, Z, 0,
        Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, 0, 0, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, 0, Z,
        0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z,
        0, Z, 0, Z, 0, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, 0, 0, 0, Z, 0, Z, 0,
        Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, 0, 0, 0, 0, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0,
        0, 0, 0, 0, 0, 0, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, Z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Z, 0, Z,
        0, Z, 0, Z, 0, Z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Z, 0, Z, 0, Z, 0, Z, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, Z, 0, Z, 0, Z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Z, 0,
        Z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Z,
    }};
#undef Z

#endif // ICONS_H

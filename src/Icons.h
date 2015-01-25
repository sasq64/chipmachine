#ifndef ICONS_H
#define ICONS_H

#include <vector>

#define Z 0xff444488
static const std::vector<unsigned> heart_icon = { 0,Z,Z,0,Z,Z,0,0,
                                 Z,Z,Z,Z,Z,Z,Z,0,
                                 Z,Z,Z,Z,Z,Z,Z,0,
                                 0,Z,Z,Z,Z,Z,0,0,
                                 0,0,Z,Z,Z,0,0,0,
                                 0,0,0,Z,0,0,0,0 };

#undef Z
#define Z 0xff44cccc
static const std::vector<unsigned> net_icon = { 0,0,Z,Z,Z,0,0,0,
                               0,0,Z,0,Z,0,0,0,
                               Z,Z,Z,0,Z,0,Z,Z,
                               0,0,0,0,Z,0,Z,0,
                               0,0,0,0,Z,Z,Z,0 };
#undef Z


#define Z 0xffffcccc
static const std::vector<unsigned> volume_icon = {
                               0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,Z,
                               0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,Z,0,Z,
                               0,0,0,0,0,0,0,0,0,0,0,0,0,0,Z,0,Z,0,Z,
                               0,0,0,0,0,0,0,0,0,0,0,0,Z,0,Z,0,Z,0,Z,
                               0,0,0,0,0,0,0,0,0,0,Z,0,Z,0,Z,0,Z,0,Z,
                               0,0,0,0,0,0,0,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,
                               0,0,0,0,0,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,
                               0,0,0,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,
                               0,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,
                               Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,
                               0,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,
                               0,0,0,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,
                               0,0,0,0,0,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,
                               0,0,0,0,0,0,0,0,Z,0,Z,0,Z,0,Z,0,Z,0,Z,
                               0,0,0,0,0,0,0,0,0,0,Z,0,Z,0,Z,0,Z,0,Z,
                               0,0,0,0,0,0,0,0,0,0,0,0,Z,0,Z,0,Z,0,Z,
                               0,0,0,0,0,0,0,0,0,0,0,0,0,0,Z,0,Z,0,Z,
                               0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,Z,0,Z,
                               0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,Z,
                           };
#undef Z


#endif // ICONS_H

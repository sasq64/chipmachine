#ifndef PLAYER_FACTORY
#define PLAYER_FACTORY

#include <coreutils/utils.h>
class ChipPlayer;

/** Interface to create a player from a file
*/
class PlayerFactory {
public:
	virtual ChipPlayer *fromFile(utils::File &f) = 0;
	virtual bool canHandle(const std::string &name) = 0;	
};

#endif // PLAYER_FACTORY
#ifndef PLAYER_FACTORY
#define PLAYER_FACTORY

#include "utils.h"
class ChipPlayer;

class PlayerFactory {
public:
	virtual ChipPlayer *fromFile(utils::File &f) = 0;
	virtual bool canHandle(const std::string &name) = 0;	
};

#endif // PLAYER_FACTORY
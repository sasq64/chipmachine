#ifndef URL_PLAYER_H
#define URL_PLAYER_H

#include "ChipPlayer.h"
#include "WebGetter.h"
#include "PlayerFactory.h"

class URLPlayer : public ChipPlayer {
public:

	URLPlayer(const std::string &url, PlayerFactory *playerFactory);
	int getSamples(int16_t *target, int noSamples) override;
	void seekTo(int song, int seconds) override;
	std::string getMeta(const std::string &what) override;

	void onMeta(Callback callback) override {
		LOGD("Setting callback in %p", this);
		if(currentPlayer)
			currentPlayer->onMeta(callback);
		else
			callbacks.push_back(callback);
	}


private:
	WebGetter webGetter;
	ChipPlayer *currentPlayer;
	WebGetter::Job *urlJob;
	std::mutex m;
	PlayerFactory *playerFactory;
};

#endif // URL_PLAYER_H
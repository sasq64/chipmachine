#ifndef URL_PLAYER_H
#define URL_PLAYER_H

#include "log.h"

#include "ChipPlayer.h"
#include "WebGetter.h"
#include "PlayerFactory.h"

#include <memory>

class URLPlayer : public ChipPlayer {
public:

	URLPlayer(const std::string &url, PlayerFactory *playerFactory);

	virtual ~URLPlayer() {
		//LOGD("URLPlayer destroy");
	}

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
	std::unique_ptr<ChipPlayer> currentPlayer;
	std::vector<std::unique_ptr<WebGetter::Job>> urlJobs;
	std::mutex m;
	PlayerFactory *playerFactory;
};

#endif // URL_PLAYER_H
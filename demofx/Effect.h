#ifndef EFFECT_H
#define EFFECT_H

#include "../src/renderable.h"

#include <tween/tween.h>
#include <grappix/grappix.h>

namespace demofx {

class Effect {
public:

	//Effect() : Renderable(grappix::screenptr) {}
	//Effect(grappix::RenderTarget &target) : Renderable(grappix::screenptr) {}

	virtual void render(uint32_t delta) = 0;

	virtual void set(const std::string &what, const std::string &val, float seconds = 0.0) {}
	virtual void set(const std::string &what, int val, float seconds = 0.0) {}

	virtual void fadeIn(float seconds = 1.0) {
		tween::Tween::make().to(alpha, 1.0).seconds(seconds);
	}
	virtual void fadeOut(float seconds = 1.0) {
		tween::Tween::make().to(alpha, 0.0).seconds(seconds);	
	}

	virtual void resize(int w,  int h) {}

	union {
		struct {
			float alpha;
			float x;
			float y;
		};
		float data[3];
	};

	virtual float& operator[](const int &i) { return data[i]; }

};

}

#endif // EFFECT_H

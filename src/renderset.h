#ifndef RENDER_SET_H
#define RENDER_SET_H

#include "renderable.h"

#include <memory>
#include <coreutils/vec.h>
#include <coreutils/utils.h>
#include <grappix/grappix.h>

class RenderSet {
public:

	RenderSet(std::shared_ptr<grappix::RenderTarget> target = grappix::screenptr) : target(target) {}

	void add(std::shared_ptr<Renderable> r);
	void add(Renderable *r);
	void remove(std::shared_ptr<Renderable> r);
	void remove(Renderable *r);

	void render(uint32_t delta) const {
		for(auto &r : fields) {
			if(r->visible())
				r->render(delta);
		}
	}
private:

	std::vector<Pointer<Renderable>> fields;
	std::shared_ptr<grappix::RenderTarget> target;

};

#endif // RENDER_SET_H

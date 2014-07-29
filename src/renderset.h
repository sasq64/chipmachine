#ifndef RENDER_SET_H
#define RENDER_SET_H

#include "renderable.h"

#include <memory>
#include <coreutils/vec.h>
#include <grappix/grappix.h>

class RenderSet {
public:

	RenderSet(std::shared_ptr<grappix::RenderTarget> target = grappix::screenptr) : target(target) {}

	void add(std::shared_ptr<Renderable> r);

	void remove(std::shared_ptr<Renderable> r) {
		auto it = fields.begin();
		while(it != fields.end()) {
			if(r.get() == it->get()) {
				(*it)->setParent(nullptr);
				it = fields.erase(it);
			} else
				it++;
		}
	}

	void remove(Renderable *r) {
		auto it = fields.begin();
		while(it != fields.end()) {
			if(r == it->get()) {
				(*it)->setParent(nullptr);
				it = fields.erase(it);
			} else
				it++;
		}
	}

	void render(uint32_t delta) {
		for(auto &r : fields) {
			if(r->visible())
				r->render(delta);
		}
	}

	std::vector<std::shared_ptr<Renderable>> fields;
	std::shared_ptr<grappix::RenderTarget> target;

};

#endif // RENDER_SET_H

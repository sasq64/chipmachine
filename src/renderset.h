#ifndef RENDER_SET_H
#define RENDER_SET_H

#include "renderable.h"

#include <memory>
#include <coreutils/vec.h>
#include <grappix/grappix.h>

class RenderSet {
public:

	void add(std::shared_ptr<Renderable> r) {
		fields.push_back(r);
	}

	void remove(std::shared_ptr<Renderable> r) {
		auto it = fields.begin();
		while(it != fields.end()) {
			if(r.get() == it->get())
				it = fields.erase(it);
			else
				it++;
		}
	}

	void render(grappix::RenderTarget &target, uint32_t delta) {
		for(auto &r : fields)
			r->render(target, delta);
	}

	std::vector<std::shared_ptr<Renderable>> fields;

};

#endif // RENDER_SET_H

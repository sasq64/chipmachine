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

	// Can hold smart_ptr or raw pointer
	template <typename T> struct Pointer {
		Pointer(std::shared_ptr<T> p) : sptr(p), ptr(p.get()) {}
		Pointer(T *p) : ptr(p) {}
		std::shared_ptr<T> sptr;
		T *ptr = nullptr;
		T* operator ->() const { return ptr; }
		operator std::shared_ptr<T>() const { return sptr; }
	};

	std::vector<Pointer<Renderable>> fields;
	std::shared_ptr<grappix::RenderTarget> target;

};

#endif // RENDER_SET_H

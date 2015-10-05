#include "renderset.h"

using namespace std;

void RenderSet::add(shared_ptr<Renderable> r) {
	r->setTarget(target);
	fields.push_back(r);
	r->setParent(this);
}

void RenderSet::add(Renderable *r) {
	r->setTarget(target);
	fields.push_back(r);
	r->setParent(this);
}

void RenderSet::remove(shared_ptr<Renderable> r) {
	auto it = fields.begin();
	while(it != fields.end()) {
		if(r.get() == it->get()) {
			(*it)->setParent(nullptr);
			it = fields.erase(it);
		} else
			it++;
	}
}

void RenderSet::remove(Renderable *r) {
	auto it = fields.begin();
	while(it != fields.end()) {
		if(r == it->get()) {
			(*it)->setParent(nullptr);
			it = fields.erase(it);
		} else
			it++;
	}
}

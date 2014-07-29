#include "renderset.h"

using namespace std;

void RenderSet::add(shared_ptr<Renderable> r) {
	r->setTarget(target);
	fields.push_back(r);
	r->setParent(this);
}

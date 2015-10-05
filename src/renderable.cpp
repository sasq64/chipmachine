#include "renderable.h"
#include "renderset.h"

void Renderable::remove() {
	if(parent)
		parent->remove(this);
	parent = nullptr;
	// do_remove = true;
}

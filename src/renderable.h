#ifndef RENDERABLE_H
#define RENDERABLE_H

#include <grappix/render_target.h>
//class grappix::RenderTarget;

class Renderable
{
public:
	virtual void render(grappix::RenderTarget &target, uint32_t delta) = 0;
};


#endif // RENDERABLE_H

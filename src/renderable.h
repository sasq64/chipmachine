#ifndef RENDERABLE_H
#define RENDERABLE_H

#include <grappix/render_target.h>
//class grappix::RenderTarget;

class Renderable
{
public:
	virtual void render(grappix::RenderTarget &target, uint32_t delta) = 0;
	virtual void visible(bool b) { is_visible = b; }
	virtual bool visible() { return is_visible; }
protected:
	bool is_visible = true;
};


#endif // RENDERABLE_H

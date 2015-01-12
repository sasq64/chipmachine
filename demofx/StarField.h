#include "Effect.h"
#include <grappix/grappix.h>

namespace demofx {

class StarField : public Effect {
public:
	StarField(grappix::RenderTarget &target) : target(target) {
		image::bitmap bm(grappix::screen.width(), grappix::screen.height());
		bm.clear(0x00000000);
		for(unsigned y=0; y<bm.height(); y++) {
			auto x = rand() % bm.width();
			bm[y*bm.width()+x] = bm[y*bm.width()+x + 1] = 0xff666666;
			bm[y*bm.width()+x + 2] = 0xff444444;
		}
		starTexture = grappix::Texture(bm);

		starProgram = grappix::get_program(grappix::TEXTURED_PROGRAM).clone();
		starProgram.setFragmentSource(starShaderF);
	}

	virtual void render(uint32_t delta) override {
		starProgram.use();
		if(starPos > 1.0) starPos -= 1.0;
		starProgram.setUniform("alpha", alpha);
		starProgram.setUniform("scrollpos", starPos += (0.3 / target.width()));
		target.draw(starTexture, starProgram);
	};

private:

	grappix::RenderTarget &target;

	const std::string starShaderF = R"(
		uniform sampler2D sTexture;
		uniform float scrollpos; // 0 -> 1

		uniform float alpha;

		varying vec2 UV;

		void main() {
			float m = mod(gl_FragCoord.y, 3.0) * alpha;
			float uvx = mod(UV.x + scrollpos * m, 1.0);
			gl_FragColor = m * texture2D(sTexture, vec2(uvx, UV.y));
		}
	)";
	grappix::Texture starTexture;
	grappix::Program starProgram;
	float starPos = 0.0;

};

}
#include "Effect.h"
#include <grappix/grappix.h>

namespace demofx {

class StarField : public Effect {
public:
	StarField(grappix::RenderTarget &target) : target(target) {
		resize(grappix::screen.width(), grappix::screen.height());

		starProgram = grappix::get_program(grappix::TEXTURED_PROGRAM).clone();
		starProgram.setFragmentSource(starShaderF);
	}

	virtual void render(uint32_t delta) override {
		starProgram.use();
		if(starPos > 1.0) starPos -= 1.0;
		starProgram.setUniform("alpha", alpha);
		starProgram.setUniform("scrollpos", starPos += (0.3 / target.width()));
		target.draw(starTexture, 0, 0, starTexture.width(), starTexture.height()*2, nullptr, starProgram);
	};

	void resize(int w, int h) override {
		image::bitmap bm(w, h/2);
		bm.clear(0x00000000);
		for(unsigned y=0; y<bm.height(); y++) {
			auto x = rand() % bm.width();
			bm[y*bm.width()+x] = bm[y*bm.width()+x + 1] = 0x66666666;
			bm[y*bm.width()+x + 2] = 0x44444444;
		}
		starTexture = grappix::Texture(bm);
	};

private:

	grappix::RenderTarget &target;

	const std::string starShaderF = R"(
		uniform sampler2D sTexture;
		uniform float scrollpos; // 0 -> 1

		uniform float alpha;

		varying vec2 UV;

		void main() {
			float m = floor(mod(gl_FragCoord.y, 6.0)/2.0 + 1.0);
			float uvx = mod(UV.x + scrollpos * m, 1.0);
			gl_FragColor = m  * texture2D(sTexture, vec2(uvx, UV.y));
		}
	)";
	grappix::Texture starTexture;
	grappix::Program starProgram;
	float starPos = 0.0;

};

}
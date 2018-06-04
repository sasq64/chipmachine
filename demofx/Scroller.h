#ifndef SCOLLER_H
#define SCOLLER_H

#include "Effect.h"

#include <coreutils/file.h>
#include <grappix/grappix.h>
#include <coreutils/environment.h>

namespace demofx {

class Scroller : public Effect {
public:
	Scroller(grappix::RenderTarget &target) : target(target), scr(grappix::screen.width()+10, 180/4) {
		//font = grappix::Font("data/ObelixPro.ttf", 24, 512 | grappix::Font::DISTANCE_MAP);
		program = grappix::get_program(grappix::TEXTURED_PROGRAM).clone();

		grappix::Resources::getInstance().load<std::string>(Environment::getCacheDir() / "sine_shader.glsl",
			[=](std::shared_ptr<std::string> source) {
				try {
					program.setFragmentSource(*source);
				} catch(grappix::shader_exception &e) {
					LOGD("ERROR");
				}
			}, sineShaderF);


		fprogram = grappix::get_program(grappix::FONT_PROGRAM_DF).clone();
		fprogram.setFragmentSource(fontShaderF);
		//font.set_program(fprogram);
	}

	void resize(int w, int h) override {
		if(w > 8 && h > 8)
			scr = grappix::Texture(w+10, h);
	}
	virtual void set(const std::string &what, const std::string &val, float seconds = 0.0) override {
		if(what == "font") {
			font = grappix::Font(val, 24, 512 | grappix::Font::DISTANCE_MAP);
			//font.set_program(fprogram);
		} else {
			scrollText = val;
			LOGD("SCROLL: %s", scrollText);
			xpos = target.width() + 100;
			scrollLen = font.get_width(val, scrollsize);
		}
	}

	virtual void render(uint32_t delta) override {
		if(alpha <= 0.01)
			return;
		if(xpos < -scrollLen)
			xpos = target.width() + 100;

		scr.clear(0x00000000);
		scr.text(font, scrollText, xpos-=scrollspeed, 10, 0xffffff | ((int)(alpha*255) << 24), scrollsize);
		program.use();
		static float uvs[] = { 0,0,1,0,0,1,1,1 };
		target.draw(scr, 0.0f, scrolly, scr.width(), scr.height(), uvs, program);
	}

	float alpha = 1.0;

	int scrollspeed = 16;
	int scrolly = 0;
	float scrollsize = 4.0;

private:
	grappix::RenderTarget& target;
	grappix::Font font;
	grappix::Program program;
	grappix::Program fprogram;
	int xpos = -9999;
	grappix::Texture scr;
	std::string scrollText;
	int scrollLen;


	const std::string sineShaderF = R"(
		uniform sampler2D sTexture;

		const vec4 color0 = vec4(0.7, 1.0, 0.5, 1.0);
		const vec4 color1 = vec4(0.2, 0.0, 1.0, 1.0);

		varying vec2 UV;

		void main() {

			vec4 rgb = mix(color0, color1, UV.y);
			// MODIFY UV HERE
			vec4 color = texture2D(sTexture, UV);
			// MODIFY COLOR HERE
			gl_FragColor = rgb * color;
		}
	)";


	const std::string fontShaderF = R"(
		uniform vec4 vColor;
		uniform vec4 vScale;
		uniform sampler2D sTexture;
		//uniform float smoothing;
		varying vec2 UV;

		vec3 glyph_color    = vec3(0.0,1.0,0.0);
		const float glyph_center   = 0.50;
		vec3 outline_color  = vec3(0.0,0.0,1.0);
		const float outline_center = 0.58;
		vec3 glow_color     = vec3(1.0, 1.0, 0.0);
		const float glow_center    = 1.0;

		void main() {
			float dist = texture2D(sTexture, UV).a;
	#ifdef GL_ES
			float smoothing = 1.0 / (vScale.x * 16.0);
			float alpha = smoothstep(glyph_center-smoothing, glyph_center+smoothing, dist);
	#else
			float width = fwidth(dist);
			float alpha = smoothstep(glyph_center-width, glyph_center+width, dist);
			//float alpha = dist;
	#endif

			//gl_FragColor = vec4(1.0, 0.0, 0.0, alpha);
			//vec3 rgb = mix(vec3(0,0,0), vec3(1.0,0.0,0.0), dist);
			//gl_FragColor = vec4(rgb, 1.0);//floor(dist + 0.500));

			gl_FragColor = vec4(vColor.rgb, vColor.a * alpha);

			//gl_FragColor = vec4(1.0, 0.0, 0.0, floor(dist + 0.500));
			//gl_FragColor += vec4(0.0, 1.0, 0.0, floor(dist + 0.533));

			//float mu = smoothstep(outline_center-width, outline_center+width, dist);
			//vec3 rgb = mix(outline_color, glyph_color, mu);
			//gl_FragColor = vec4(rgb, max(alpha,mu));

			//vec3 rgb = mix(glow_color, vec3(1.0,1.0,1.0), alpha);
			//float mu = smoothstep(glyph_center, glow_center, sqrt(dist));
			//gl_FragColor = vec4(rgb, mu);//max(alpha,mu));

		}

	)";


};

}

#endif // SCOLLER_H

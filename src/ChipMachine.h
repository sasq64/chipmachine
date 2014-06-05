#ifndef CHIP_MACHINE_H
#define CHIP_MACHINE_H

#include "MusicDatabase.h"
#include "MusicPlayerList.h"
#include "TextScreen.h"
#include "SongInfoField.h"

#include "TelnetInterface.h"

#include "MainScreen.h"
#include "SearchScreen.h"

#include <tween/tween.h>
#include <grappix/grappix.h>
#include <lua/luainterpreter.h>

#include <coreutils/utils.h>

#include <cstdio>
#include <vector>
#include <string>
#include <memory>

namespace chipmachine {

class Effect {
public:
	virtual void render(uint32_t delta) = 0;
	virtual void fadeIn() {
	}
	virtual void fadeOut() {
	}
};

class StarField : public Effect {
public:
	StarField(grappix::RenderTarget &target) : target(target) {
		image::bitmap bm(screen.width(), screen.height());
		bm.clear(0x00000000);
		for(int y=0; y<bm.height(); y++) {
			auto x = rand() % bm.width();
			bm[y*bm.width()+x] = bm[y*bm.width()+x + 1] = 0xff666666;
			bm[y*bm.width()+x + 2] = 0xff444444;
		}
		starTexture = Texture(bm);

		starProgram = get_program(TEXTURED_PROGRAM).clone();
		starProgram.setFragmentSource(starShaderF);
	}

	virtual void render(uint32_t delta) override {
		starProgram.use();
		starProgram.setUniform("scrollpos", starPos += (0.3 / target.width()));
		target.draw(starTexture, starProgram);
	};
private:

	grappix::RenderTarget &target;

	const std::string starShaderF = R"(
	#ifdef GL_ES
		precision mediump float;
	#endif
		uniform sampler2D sTexture;
		uniform float scrollpos; // 0 -> 1

		varying vec2 UV;

		void main() {
			float m = mod(gl_FragCoord.y, 3.0);
			float uvx = mod(UV.x + scrollpos * m, 1.0);
			gl_FragColor = m * texture2D(sTexture, vec2(uvx, UV.y));
		}
	)";
	grappix::Texture starTexture;
	grappix::Program starProgram;
	float starPos = 0.0;

};

class Scroller : public Effect {
public:
	Scroller(grappix::RenderTarget &target) : target(target), scr(screen.width()+200, 440) {
		font = Font("data/ObelixPro.ttf", 24, Font::UPPER_CASE | Font::DISTANCE_MAP);
		program = get_program(TEXTURED_PROGRAM).clone();
		program.setFragmentSource(sineShaderF);

		fprogram = get_program(FONT_PROGRAM_DF).clone();
		fprogram.setFragmentSource(fontShaderF);
		font.setProgram(fprogram);
	}

	virtual void render(uint32_t delta) override {
		if(xpos < -3600)
			xpos = target.width() + 200;
		scr.clear(0x00000000);
		scr.text(font, "BALLS ON THE SCREEN!!", xpos-=4, 10, 0xe080c0ff, 4.0);
		program.use();
		static float uvs[] = { 0,0,1,0,0,1,1,1 };
		target.draw(scr, 0.0f, 350.0f, scr.width(), scr.height(), uvs, program);
	}
private:
	grappix::RenderTarget target;
	Font font;
	Program program;
	Program fprogram;
	int xpos = -9999;
	Texture scr;

	const std::string sineShaderF = R"(
	#ifdef GL_ES
		precision mediump float;
	#endif
		uniform sampler2D sTexture;

		const vec4 color0 = vec4(0.0, 1.0, 0.0, 1.0);
		const vec4 color1 = vec4(1.0, 0.3, 0.3, 1.0);

		varying vec2 UV;

		void main() {
			vec4 rgb = mix(color0, color1, UV.y);
			gl_FragColor = rgb * texture2D(sTexture, UV);
		}
	)";


	const std::string fontShaderF = R"(
	#ifdef GL_ES
		precision mediump float;
	#endif
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


class ChipMachine {
public:
	ChipMachine();
	void initLua();
	void play(const SongInfo &si);
	void update();
	void render(uint32_t delta);

private:

	ModlandDatabase modland;

	MusicPlayerList player;

	MainScreen mainScreen;
	SearchScreen searchScreen;

	int currentScreen = 0;

	std::unique_ptr<TelnetInterface> telnet;

	utils::vec2i tv0 = { 80, 54 };
	utils::vec2i tv1 = { 636, 520 };

	grappix::Color spectrumColor { 0xffffffff };
	grappix::Color spectrumColorMain { 0xff00aaee };
	grappix::Color spectrumColorSearch { 0xff111155 };
	double spectrumHeight = 20.0;
	int spectrumWidth = 24;
	utils::vec2i spectrumPos;
	std::vector<uint8_t> eq;


	LuaInterpreter lua;
/*
	grappix::Texture starTexture;
	grappix::Program starProgram;
	float starPos = 0.0;
*/
	StarField starEffect;
	Scroller scrollEffect;

};

}


#endif // CHIP_MACHINE_H

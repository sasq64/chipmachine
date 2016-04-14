#include "MusicBars.h"
#include <grappix/GL_Header.h>
#include <string>

//#ifndef RASPBERRYPI
//#define PIXEL_EQ
//#endif

using namespace grappix;
using namespace utils;

static const std::string eqShaderV = R"(
	attribute vec4 vertex;
	attribute vec2 uv;

	uniform mat4 matrix;
	varying vec2 UV;

	void main() {
		gl_Position = matrix * vertex;
		//vec4 r = matrix * vec4(uv, 0.0, 1.0);
		UV = uv;
		UV.y = uv.y * -10.0 * matrix[1].y;
	}
)";

static const std::string eqShaderF = R"(
	uniform float slots[25];
	uniform float specx;
	uniform float specy;
	uniform float specw;
	uniform float spech;

	const vec4 color0 = vec4(0.0, 0.0, 0.0, 1.0);
	const vec4 color1 = vec4(0.0, 0.4, 0.0, 1.0);
	const vec4 color2 = vec4(0.8, 0.8, 0.0, 1.0);

	varying vec2 UV;

	uniform sampler2D sTexture;

	void main() {

		vec4 c = texture2D(sTexture, UV);

		// int(h) is the eq slot to read
		float h = 24.0 * (gl_FragCoord.x - specx) / specw;

		// float f = fract(h);

		// Linear interpolation between slots
		// float y = mix(slots[int(h)], slots[int(h)+1], f);
		float y = slots[int(h)]; // 0 -> spech

		// Blend from color0 -> color1 -> color 0 over y+4 -> y-4

		float fy = gl_FragCoord.y - specy;

		//vec4 rgb = mix(color1, color2, (y - fy) / specy);
		//vec4 rgb = mix(color0, color2, smoothstep(y + 2.0, y - 2.0, fy));

		vec4 rgb = mix(color0, c, smoothstep(y + 1.0, y - 1.0, fy));

		//vec4 rgb = mix(color1, color2, smoothstep(fy + 20.0, fy - 20.0, y));
		//rgb = mix(rgb, color0, smoothstep(fy - 2.0, fy - 4.0, y));


		//rgb = rgb * mod(h, 1.0);
		//rgb = rgb * mod(UV.y * 256.0, 2.0);

		// MODIFY UV HERE
		//vec4 color = texture2D(sTexture, UV);
		// MODIFY COLOR HERE
		gl_FragColor = vec4(rgb.xyz, c.a);
	}
)";

MusicBars::MusicBars() {}

void MusicBars::setup(int w, int h, int slots) {

	spectrumWidth = w;
	spectrumHeight = h;

	interval = 6;
	int gap = 1;
	if(spectrumWidth < 18) {
		interval = 4;
		gap = 0;
	}

	image::bitmap eqbar(spectrumWidth * 24, spectrumHeight);
	Color col(0xff66ff66);
	Color toc0(0xff008888);
	Color toc1(0xff000066);
	int h2 = eqbar.height() / 2;
	Color deltac = (toc0 - col) / (float)h2;
	// auto eqtween = Tween::make().to(c, 0xffff0000).seconds(eqbar.height());

	for(int y = eqbar.height() - 1; y > 0; y--) {
		for(int x = 0; x < eqbar.width(); x++) {
			eqbar[x + y * eqbar.width()] =
			    (y % interval > gap) && (x % spectrumWidth != 0) ? (uint32_t)col : 0x00000000;
		}
		col = col + deltac;
		if(y == h2) {
			col = toc0;
			deltac = (toc1 - col) / (float)h2;
		}
	}
	eqTexture = Texture(eqbar);
	glBindTexture(GL_TEXTURE_2D, eqTexture.id());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	if(!eqProgram) {
		eqProgram = grappix::get_program(grappix::TEXTURED_PROGRAM).clone();
	#ifdef PIXEL_EQ
		eqProgram.setFragmentSource(eqShaderF);
	#endif
		// eqProgram.setVertexSource(eqShaderV);
	}
}

void MusicBars::render(const utils::vec2i &spectrumPos, const grappix::Color &spectrumColor,
                       const std::vector<uint8_t> &eq) {

#ifdef PIXEL_EQ
	static std::vector<float> fSlots(25);
	for(int i = 0; i < 24; i++) {
		fSlots[i] = spectrumHeight * eq[i] / 256.0;
	}
	fSlots[24] = fSlots[23];

	eqProgram.use();
	eqProgram.setUniform("slots", &fSlots[0], 25);
	eqProgram.setUniform("specx", spectrumPos.x);
	eqProgram.setUniform("specy", screen.height() - spectrumPos.y);
	eqProgram.setUniform("specw", spectrumWidth * 24);
	eqProgram.setUniform("spech", spectrumHeight);
	screen.draw(eqTexture, spectrumPos.x, spectrumPos.y - spectrumHeight, spectrumWidth * 24,
	            spectrumHeight, nullptr, eqProgram);
#else
	screen.draw(eqTexture, spectrumPos.x, spectrumPos.y - spectrumHeight, spectrumWidth * 24,
	            spectrumHeight, nullptr, spectrumColor);
	for(auto i : count_to(eq.size())) {
		int h = (spectrumHeight * eq[i] / (256 * interval));
		h *= interval - 1;
		screen.rectangle(spectrumPos.x + (spectrumWidth)*i, spectrumPos.y - spectrumHeight,
		                 spectrumWidth, spectrumHeight - h, 0xff000000);
	}
#endif
}

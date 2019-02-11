
#include <coreutils/vec.h>
#include <grappix/grappix.h>

class MusicBars
{
public:
    MusicBars();
    void setup(int w, int h);
    void render(const utils::vec2i& spectrumPos,
                const grappix::Color& spectrumColor,
                const std::vector<uint8_t>& eq);

private:
    grappix::Texture eqTexture;
    grappix::Program eqProgram;

    int spectrumHeight = 20;
    int spectrumWidth = 24;
    int interval = 6;
};

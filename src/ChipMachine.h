#pragma once

#include "Dialog.h"
#include "LineEdit.h"
#include "MusicBars.h"
#include "MusicDatabase.h"
#include "MusicPlayerList.h"
#include "SongInfoField.h"
#include "TelnetInterface.h"
#include "TextField.h"
#include "state_machine.h"

#include "../demofx/Scroller.h"
#include "../demofx/StarField.h"
#include "../sol2/sol.hpp"

#include <coreutils/utils.h>
#include <fft/spectrum.h>
#include <grappix/grappix.h>
#include <grappix/gui/list.h>
#include <grappix/gui/renderset.h>
#include <tween/tween.h>

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace chipmachine {

class Icon : public Renderable
{
public:
    Icon() = default;

    Icon(std::shared_ptr<grappix::Texture> tx, float x, float y, float w,
         float h)
        : texture(tx), rec(x, y, w, h)
    {}

    Icon(const image::bitmap& bm, int x = 0, int y = 0)
        : rec(x, y, bm.width(), bm.height())
    {
        setBitmap(bm);
    }

    Icon(const image::bitmap& bm, float x, float y, float w, float h)
        : rec(x, y, w, h)
    {
        setBitmap(bm);
    }

    void render(std::shared_ptr<grappix::RenderTarget> target,
                uint32_t delta) override
    {
        if (!texture || (color >> 24) == 0) return;
        target->draw(*texture, rec.x, rec.y, rec.w, rec.h, nullptr, color);
    }

    void setBitmap(const image::bitmap& bm, bool filter = false)
    {
        texture = std::make_shared<grappix::Texture>(bm);
        rec.w = bm.width();
        rec.h = bm.height();
        glBindTexture(GL_TEXTURE_2D, texture->id());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                        filter ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        filter ? GL_LINEAR : GL_NEAREST);
    }

    void clear() { texture = nullptr; }

    void setArea(const grappix::Rectangle& r) { rec = r; }

    grappix::Color color{ 0xffffffff };
    grappix::Rectangle rec;

private:
    std::shared_ptr<grappix::Texture> texture;
};

class ChipMachine
{
public:
    using Color = grappix::Color;

    void renderSong(const grappix::Rectangle& rec, int y, uint32_t index,
                    bool hilight);
    void renderCommand(grappix::Rectangle& rec, int y, uint32_t index,
                       bool hilight);

    ChipMachine(utils::path const& workDir, RemoteLoader& rl,
                MusicPlayerList& mpl, MusicDatabase& mdb);
    ~ChipMachine();

    void initLua();
    void layoutScreen();
    void play(const SongInfo& si);
    void update();
    void render(uint32_t delta);

    enum ToastType
    {
        WHITE,
        ERROR,
        NORMAL,
        STICKY
    };

    enum Shuffle
    {
        All = 0,
        Format = 1,
        Composer = 2,
        Collection = 4,
    };

    void toast(const std::string& txt, ToastType type = NORMAL);
    void removeToast();

    void setScrolltext(const std::string& txt);
    void shuffleSongs(int what, int limit);

    void shuffleFavorites();
    MusicPlayerList& musicPlayer() { return player; }
    void playSongs(std::vector<SongInfo> const& songs);
    void playNamed(const std::string& what) { namedToPlay = what; }

private:
    enum Screen
    {
        NO_SCREEN = -1,
        MAIN_SCREEN = 0,
        SEARCH_SCREEN = 1,
        COMMAND_SCREEN = 2,
    };

    static const uint32_t SHIFT = 0x10000;
    static const uint32_t CTRL = 0x20000;
    static const uint32_t ALT = 0x40000;

    void setVariable(const std::string& name, int index,
                     const std::string& val);

    void showScreen(Screen screen);
    SongInfo getSelectedSong();

    void setupRules();
    void setupCommands();
    void updateKeys();
    void updateFavorite();
    void updateNextField();
    void updateLists()
    {

        int y = resultFieldTemplate.pos.y + 5;

        songList.setArea(grappix::Rectangle(topLeft.x, y,
                                            grappix::screen.width() - topLeft.x,
                                            downRight.y - topLeft.y - y));
        commandList.setArea(grappix::Rectangle(
            topLeft.x, y, grappix::screen.width() - topLeft.x,
            downRight.y - topLeft.y - y));
    }

    static inline const std::vector<std::string> key_names = {
        "UP",  "DOWN",   "LEFT",     "RIGHT",  "ENTER", "ESCAPE", "BACKSPACE",
        "TAB", "PAGEUP", "PAGEDOWN", "DELETE", "HOME",  "END",    "F1",
        "F2",  "F3",     "F4",       "F5",     "F6",    "F7",     "F8",
        "F9",  "F10",    "F11",      "F12"
    };

    void addKey(uint32_t key, statemachine::Condition const& cond,
                std::string const& cmd);

    void addKey(std::vector<uint32_t> const& events,
                statemachine::Condition const& cond, std::string const& cmd)
    {
        for (auto& e : events)
            addKey(e, cond, cmd);
    }

    void addKey(std::vector<uint32_t> const& events, std::string const& cmd)
    {
        addKey(events, statemachine::ALWAYS_TRUE, cmd);
    }

    void addKey(uint32_t key, std::string const& cmd)
    {
        addKey(key, statemachine::ALWAYS_TRUE, cmd);
    }

    void clearCommand()
    {
        matchingCommands.resize(commands.size());
        int i = 0;
        for (auto& c : commands)
            matchingCommands[i++] = &c;
    }

    bool haveSelection()
    {
        return songList.selected() >= 0 &&
               (songList.selected() < songList.size());
    }

    void nextScreenshot();

    utils::path workDir;

    RemoteLoader& remoteLoader;
    MusicPlayerList& player;
    MusicDatabase& musicDatabase;

    Screen lastScreen = MAIN_SCREEN;
    Screen currentScreen = MAIN_SCREEN;

    std::unique_ptr<TelnetInterface> telnet;

    // Aera of screen used for UI (defaults are for TV with overscan)
    utils::vec2i topLeft = { 80, 54 };
    utils::vec2i downRight = { 636, 520 };

    grappix::Font font;
    grappix::Font listFont;

    int spectrumHeight = 20;
    int spectrumWidth = 24;
    utils::vec2i spectrumPos;
    std::vector<uint8_t> eq;
    SpectrumAnalyzer fft;
    std::array<uint16_t, SpectrumAnalyzer::eq_slots> spectrum;

    uint32_t bgcolor = 0;
    bool starsOn = true;

    sol::state lua;

    demofx::StarField starEffect;
    demofx::Scroller scrollEffect;

    // OVERLAY AND ITS RENDERABLES

    RenderSet overlay;
    TextField toastField;

    Icon favIcon;
    Icon netIcon;
    Icon volumeIcon;
    Icon screenShotIcon;

    // MAINSCREEN AND ITS RENDERABLES
    RenderSet mainScreen;

    SongInfoField currentInfoField;
    SongInfoField nextInfoField;
    SongInfoField prevInfoField;
    SongInfoField outsideInfoField;

    TextField timeField;
    TextField lengthField;
    TextField songField;
    TextField nextField;
    TextField xinfoField;
    // TextField playlistField;

    // SEARCHSCREEN AND ITS RENDERABLES
    RenderSet searchScreen;

    LineEdit searchField;
    TextField filterField;
    TextField topStatus;
    grappix::VerticalList songList;

    TextField resultFieldTemplate;

    // COMMANDSCREEN AND ITS RENDERABLES

    RenderSet commandScreen;
    LineEdit commandField;
    grappix::VerticalList commandList;

    //

    std::string currentNextPath;
    SongInfo currentInfo;
    SongInfo dbInfo;
    int currentTune = 0;

    tween::Tween currentTween;
    bool isFavorite = false;

    grappix::Rectangle favPos;
    grappix::Rectangle volPos;

    int numLines = 20;

    tween::Tween markTween;

    Color timeColor;
    Color spectrumColor = 0xffffffff;
    Color spectrumColorMain = 0xff00aaee;
    Color spectrumColorSearch = 0xff111155;
    Color markColor = 0xff00ff00;
    Color hilightColor = 0xffffffff;

    std::shared_ptr<IncrementalQuery> iquery;

    bool haveSearchChars = false;

    statemachine::StateMachine smac;

    std::string currentPlaylistName = "Favorites";

    bool commandMode = false;

    std::shared_ptr<Dialog> currentDialog;

    std::pair<float, float> screenSize;
    int resizeDelay = 0;
    int showVolume = 0;

    bool hasMoved = false;

    bool indexingDatabase = false;

    MusicBars musicBars;
    MusicPlayerList::State playerState;
    std::string scrollText;

    struct Command
    {
        Command(const std::string& name, std::function<void()> const& fn)
            : name(name), fn(fn)
        {}
        std::string name;
        std::function<void()> fn;
        std::string shortcut;
        bool operator==(const std::string& n) { return n == name; }
        bool operator==(const Command& c) { return c.name == name; }
    };

    std::vector<Command> commands;
    std::vector<Command*> matchingCommands;
    std::mutex multiLoadLock;
    int lastKey = 0;
    bool searchUpdated = false;
    std::string filter;
    uint32_t favColor = 0x884444;

    std::string namedToPlay;
    int currentShot = -1;
    struct NamedBitmap
    {
        NamedBitmap() {}
        NamedBitmap(const std::string& name, const image::bitmap& bm)
            : name(name), bm(bm)
        {}
        std::string name;
        image::bitmap bm;
        bool operator==(const char* n) const
        {
            return strcmp(name.c_str(), n) == 0;
        }
        bool operator<(const NamedBitmap& other) const
        {
            return name < other.name;
        }
    };
    std::vector<NamedBitmap> screenshots;
    uint64_t setShotAt = 0;
    std::string currentScreenshot;
};
} // namespace chipmachine

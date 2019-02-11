#include "ChipMachine.h"
#include "Icons.h"
#include "version.h"
#include <coreutils/environment.h>
#include <coreutils/format.h>
#include <coreutils/searchpath.h>
#include <grappix/window.h>

#include <cctype>
#include <map>
#ifdef _WIN32
#    include <ShellApi.h>
#endif

using namespace grappix;
using tween::Tween;

void initYoutube(sol::state&);

std::string compressWhitespace(std::string&& m)
{
    // Turn linefeeds into spaces
    replace(begin(m), end(m), '\n', ' ');
    // Turn whitespace sequences into single spaces
    auto last = unique(begin(m), end(m),
                       [](char a, char b) { return (a | b) <= 0x20; });
    m.resize(distance(begin(m), last));
    return m;
}

std::string compressWhitespace(std::string const& text)
{
    return compressWhitespace(std::string(text));
}

namespace chipmachine {

void ChipMachine::renderSong(grappix::Rectangle const& rec, int y,
                             uint32_t index, bool hilight)
{

    static const std::map<uint32_t, uint32_t> colors = {
        { NOT_SET, 0xffff00ff }, { PLAYLIST, 0xffffff88 },
        { CONSOLE, 0xffdd3355 }, { C64, 0xffcc8844 },
        { ATARI, 0xffcccc33 },   { MP3, 0xff88ff88 },
        { M3U, 0xffaaddaa },     { YOUTUBE, 0xffff0000 },
        { PC, 0xffcccccc },      { AMIGA, 0xff6666cc },
        { PRODUCT, 0xffff88cc }, { 255, 0xff00ffff }
    };

    Color c;
    std::string text;

    auto res = iquery->getResult(index);
    auto parts = utils::split(res, "\t");
    int f = std::stol(parts[3]) & 0xff;

    if (f == PLAYLIST || f == PRODUCT) {
        if (parts[1] == "")
            text = utils::format("<%s>", parts[0]);
        else
            text = utils::format("<%s / %s>", parts[0], parts[1]);
    } else {
        if (parts[1] == "")
            text = parts[0];
        else
            text = utils::format("%s / %s", parts[0], parts[1]);
    }
    auto it = --colors.upper_bound(f);
    c = it->second;
    c = c * 0.75f;

    if (hilight) {
        static uint32_t markStartcolor = 0;
        if (markStartcolor != c) {
            markStartcolor = c;
            markColor = c;
            markTween = Tween::make()
                            .sine()
                            .repeating()
                            .from(markColor, hilightColor)
                            .seconds(1.0);
            markTween.start();
        }
        c = markColor;
    }

    grappix::screen.text(listFont, text, rec.x, rec.y, c,
                         resultFieldTemplate.scale);
}

ChipMachine::ChipMachine(utils::path const& wd, RemoteLoader& rl,
                         MusicPlayerList& mpl, MusicDatabase& mdb)
    : workDir(wd), remoteLoader(rl), player(mpl), musicDatabase(mdb),
      currentScreen(MAIN_SCREEN), eq(SpectrumAnalyzer::eq_slots),
      starEffect(screen), scrollEffect(screen)
{

    screen.setTitle("Chipmachine " VERSION_STR);
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string);
#ifdef _WIN32
    lua["WINDOWS"] = true;
#else
    lua["WINDOWS"] = false;
#endif

    utils::path binDir = (workDir / "bin");
    lua.set_function("cm_execute",
                     [binDir](std::string const& cmd) -> std::string {
                         auto cmdPath = utils::path(cmd);
                         if (!cmdPath.is_absolute()) cmdPath = binDir / cmdPath;
                         std::string output = utils::execPipe(cmdPath.string());
                         return output;
                     });

    lua.script_file((workDir / "lua" / "init.lua").string());

    initYoutube(lua);

    auto ff = workDir / "data" / "Bello.otf";
    scrollEffect.set("font", ff.string());

#ifdef ENABLE_TELNET
    telnet = std::make_unique<TelnetInterface>(player);
    telnet->start();
#endif

    nextInfoField.setAlign(1.0);
    nextField.align = 1.0;

    screenShotIcon = Icon(image::bitmap(8, 8), 100, 100);
    mainScreen.add(&screenShotIcon);

    // SongInfo fields
    mainScreen.add(&prevInfoField);
    mainScreen.add(&currentInfoField);
    mainScreen.add(&nextInfoField);
    mainScreen.add(&outsideInfoField);

    // Other text fields
    mainScreen.add(&xinfoField);
    mainScreen.add(&nextField);
    mainScreen.add(&timeField);
    mainScreen.add(&lengthField);
    mainScreen.add(&songField);

    // SEARCHSCREEN

    iquery = musicDatabase.createQuery();

    searchField.setPrompt("#");
    searchScreen.add(&searchField);
    searchField.visible(false);

    searchScreen.add(&topStatus);
    topStatus.visible(false);

    // toastField = TextField(font, "", topLeft.x, downRight.y - 134, 2.0,
    // 0x00ffffff);
    overlay.add(&toastField);

    Resources::getInstance().load<image::bitmap>(
        (Environment::getCacheDir() / "favicon.png").string(),
        [=](std::shared_ptr<image::bitmap> bitmap) {
            favIcon = Icon(heart_icon, favPos.x, favPos.y, favPos.w, favPos.h);
        },
        heart_icon);
    // favIcon = Icon(heart_icon, favPos.x, favPos.y, favPos.w, favPos.h);

    float ww = volume_icon.width() * 15;
    float hh = volume_icon.height() * 10;
    volPos = { ((float)screen.width() - ww) / 2.0f,
               ((float)screen.height() - hh) / 2.0f, ww, hh };
    volumeIcon = Icon(volume_icon, volPos.x, volPos.y, volPos.w, volPos.h);

    setupCommands();
    setupRules();

    initLua();
    layoutScreen();

    filterField = searchField;
    searchScreen.add(&filterField);
    filterField.visible(false);
    filterField.color = 0xff55ff55;

    mainScreen.add(&favIcon);
    // favIcon.visible(false);
    favIcon.color = Color(favColor);

    netIcon = Icon(net_icon, 2, 2, 8 * 3, 5 * 3);
    mainScreen.add(&netIcon);
    netIcon.visible(false);
    showVolume = 0;

    player.setAudioCallback(
        [this](int16_t* ptr, int size) { fft.addAudio(ptr, size); });

    musicBars.setup(spectrumWidth, spectrumHeight);

    LOGD("WORKDIR %s", workDir.string());
    musicDatabase.initFromLuaAsync(this->workDir);

    if (musicDatabase.busy()) {
        indexingDatabase = true;
    }

    screenSize = screen.size();
    resizeDelay = 0;

    auto listrec =
        grappix::Rectangle(topLeft.x, topLeft.y + 30 * searchField.scale,
                           screen.width() - topLeft.x,
                           downRight.y - topLeft.y - searchField.scale * 30);
    songList =
        VerticalList(listrec, numLines,
                     [=](grappix::Rectangle& rec, int y, uint32_t index,
                         bool hilight) { renderSong(rec, y, index, hilight); });

    searchScreen.add(&songList);

    commandList = VerticalList(
        listrec, numLines,
        [=](grappix::Rectangle& rec, int y, uint32_t index, bool hilight) {
            if (index < matchingCommands.size()) {
                auto cmd = matchingCommands[index];
                uint32_t c = 0xaa00cc00;
                if (hilight) {
                    static uint32_t markStartcolor = 0;
                    if (markStartcolor != c) {
                        markStartcolor = c;
                        markColor = c;
                        markTween = Tween::make()
                                        .sine()
                                        .repeating()
                                        .from(markColor, hilightColor)
                                        .seconds(1.0);
                        markTween.start();
                    }
                    c = markColor;
                }
                static int cmdPos = -1;
                if (cmdPos == -1)
                    cmdPos =
                        listFont.get_width("012345678901234567890123456789",
                                           resultFieldTemplate.scale);
                grappix::screen.text(listFont, cmd->name, rec.x, rec.y, c,
                                     resultFieldTemplate.scale);
                grappix::screen.text(listFont, cmd->shortcut, rec.x + cmdPos,
                                     rec.y, 0xffffffff,
                                     resultFieldTemplate.scale * 0.8);
            }
        });

    commandList.setTotal(commands.size());
    clearCommand();

    updateLists();

    // playlistField = TextField(listFont, "Favorites", downRight.x - 80,
    // downRight.y - 10, 0.5, 0xff888888); mainScreen.add(playlistField);

    commandScreen.add(&commandField);
    commandScreen.add(&commandList);

    scrollText = "INITIAL_TEXT";
    scrollEffect.set("scrolltext",
                     "Chipmachine " VERSION_STR
                     " -- Just type to search -- UP/DOWN to select "
                     "-- ENTER to play -- Press TAB to show all commands ---");
    starEffect.fadeIn();
}

ChipMachine::~ChipMachine()
{
#ifdef ENABLE_TELNET
    if (telnet) telnet->stop();
#endif
}

void ChipMachine::setScrolltext(std::string const& txt)
{
    scrollEffect.set("scrolltext", txt);
}

void ChipMachine::initLua()
{
    lua["set_var"] = sol::overload(
        [=](std::string const& name, uint32_t index, std::string const& val) {
            setVariable(name, index, val);
        },
        [=](std::string const& name, uint32_t index, double val) {
            setVariable(name, index, std::to_string(val));
        },
        [=](std::string const& name, uint32_t index, uint32_t val) {
            setVariable(name, index, std::to_string(val));
        });
}

void ChipMachine::layoutScreen()
{

    LOGD("LAYOUT SCREEN");
    currentTween.finish();
    currentTween = Tween();

    lua["on_layout"](screen.width(), screen.height(),
                     screen.getPPI() < 0 ? 100 : screen.getPPI());

    utils::File f(workDir / "lua" / "screen.lua");

    lua["SCREEN_WIDTH"] = screen.width();
    lua["SCREEN_HEIGHT"] = screen.height();
    lua["SCREEN_PPI"] = screen.getPPI() < 0 ? 100 : screen.getPPI();

    Resources::getInstance().load<std::string>(
        f.getName(), [=](std::shared_ptr<std::string> contents) {
            lua.script(*contents);
            lua.script(R"(
            for a,b in pairs(Settings) do
                if type(b) == 'table' then
                    for a1,b1 in ipairs(b) do
                        set_var(a, a1, b1)
                    end
                else
                    set_var(a, 0, b)
                end
            end
        )");
        });

    starEffect.resize(screen.width(), screen.height());
    scrollEffect.resize(screen.width(), 45 * scrollEffect.scrollsize);
    musicBars.setup(spectrumWidth, spectrumHeight);

    searchField.setFont(font);
    commandField.pos = searchField.pos;
    commandField.scale = searchField.scale;
    commandField.cursorH = searchField.cursorH;
    commandField.cursorW = searchField.cursorW;

    favIcon.setArea(favPos);

    float ww = volume_icon.width() * 15;
    float hh = volume_icon.height() * 10;
    volPos = { ((float)screen.width() - ww) / 2.0f,
               ((float)screen.height() - hh) / 2.0f, ww, hh };
    volumeIcon.setArea(volPos);
}

void ChipMachine::play(SongInfo const& si)
{
    player.addSong(si);
    player.nextSong();
}

void ChipMachine::updateFavorite()
{
    auto favorites = musicDatabase.getPlaylist(currentPlaylistName);
    auto favsong =
        find_if(favorites.begin(), favorites.end(), [&](SongInfo const& song) {
            return (song.path == currentInfo.path &&
                    (currentTune == song.starttune ||
                     (currentTune == currentInfo.starttune &&
                      song.starttune == -1)));
        });
    bool last = isFavorite;
    isFavorite = (favsong != favorites.end());
    uint32_t alpha = isFavorite ? 0xff : 0x00;
    favIcon.color = Color(favColor | (alpha << 24));
    // favIcon.visible(isFavorite);
}

void ChipMachine::nextScreenshot()
{
    setShotAt = utils::getms();
    if (screenshots.empty()) return;

    currentShot++;
    if (currentShot >= screenshots.size()) currentShot = 0;

    Tween::make()
        .to(screenShotIcon.color, Color(0x00000000))
        .seconds(1.0)
        .onComplete([=]() {
            if (screenshots.size() <= currentShot) {
                LOGD("Shot went away!");
                return;
            }
            auto& bm = screenshots[currentShot].bm;
            LOGD("BITMAP IS %dx%d", bm.width(), bm.height());
            screenShotIcon.setBitmap(bm, true);

            auto x = xinfoField.pos.x;
            auto y = xinfoField.pos.y + xinfoField.getHeight() + 10;

            // Available space
            auto h = scrollEffect.scrolly - y;
            auto w = screen.width() / 2;

            float d = (float)h / bm.height();
            float d2 = (float)w / bm.width();
            if (d2 < d) d = d2;
            screenShotIcon.setArea(
                grappix::Rectangle(x, y, bm.width() * d, bm.height() * d));
            Tween::make()
                .to(screenShotIcon.color, Color(0xffffffff))
                .seconds(1.0);
        });
}

void ChipMachine::updateNextField()
{
    auto psz = player.listSize();
    LOGD("####### PLAYLIST UPDATED WITH %d entries", psz);
    if (psz > 0) {
        auto info = player.getInfo(1);
        if (info.path != currentNextPath) {
            if (psz == 1)
                nextField.setText("Next");
            else
                nextField.setText(utils::format("Next (%d)", psz));
            nextInfoField.setInfo(info);
            currentNextPath = info.path;
        }
    } else if (nextField.getText() != "") {
        nextInfoField.setInfo(SongInfo());
        nextField.setText("");
    }
}
void ChipMachine::update()
{

    if (indexingDatabase) {

        static int delay = 30;
        if (delay-- == 0) toast("Indexing database", STICKY);

        if (!musicDatabase.busy()) {
            indexingDatabase = false;
            removeToast();
        } else
            return;
    }

    if (namedToPlay != "") {
        std::vector<SongInfo> target;
        SongInfo info;
        bool random = true;
        if (namedToPlay == "favorites") {
            target = musicDatabase.getPlaylist("Favorites");
        } else if (namedToPlay == "all") {
            musicDatabase.getSongs(target, info, 500, random);
        } else {
            info.path = namedToPlay + "::x";
            musicDatabase.getSongs(target, info, 500, random);
        }
        namedToPlay = "";
        for (const auto& s : target) {
            if (!utils::endsWith(s.path, ".plist")) player.addSong(s);
        }
        player.nextSong();
    }

    auto click = screen.get_click();
    if (click != Window::NO_CLICK) {
        LOGD("Clicked at %d %d\n", click.x, click.y);
    }

    if (currentDialog && currentDialog->getParent() == nullptr)
        currentDialog = nullptr;

    updateKeys();

    // DEAL WITH MUSICPLAYER STATE

    playerState = player.getState();

    if (playerState == MusicPlayerList::Playstarted) {
        timeField.add = 0;
        currentInfo = player.getInfo();
        dbInfo = player.getDBInfo();
        LOGD("MUSIC STARTING %s", currentInfo.title);
        screen.setTitle(utils::format("%s / %s (Chipmachine " VERSION_STR ")",
                                      currentInfo.title, currentInfo.composer));
        std::string m;
        if (currentInfo.metadata[SongInfo::INFO] != "") {
            m = compressWhitespace(currentInfo.metadata[SongInfo::INFO]);
        } else {
            m = compressWhitespace(player.getMeta("message"));
        }
        if (scrollText != m) {
            scrollEffect.set("scrolltext", m);
            scrollText = m;
        }

        auto shot = currentInfo.metadata[SongInfo::SCREENSHOT];

        if (shot != currentScreenshot) {
            screenShotIcon.clear();
            screenshots.clear();
            currentScreenshot = shot;

            if (shot != "") {
                auto parts = utils::split(shot, ";");
                int total = parts.size();
                auto cb = [=](utils::File f) {
                    if (currentScreenshot == "")
                        return; // We probably got a new screenshot while
                                // loading
                    int t = total;
                    if (!f) {
                        LOGD("Empty file");
                        // screenshots.emplace_back();
                    } else {
                        // LOCK_GUARD(multiLoadLock);

                        try {
                            if (utils::toLower(utils::path_extension(
                                    f.getName())) == "gif") {
                                t--;
                                for (auto& bm : image::load_gifs(f.getName())) {
                                    for (auto& px : bm) {
                                        if ((px & 0xffffff) == 0)
                                            px &= 0xffffff;
                                    }
                                    screenshots.emplace_back(f.getFileName(),
                                                             bm);
                                    t++;
                                }
                            } else {
                                auto bm = image::load_image(f.getName());
                                for (auto& px : bm) {
                                    if ((px & 0xffffff) == 0) px &= 0xffffff;
                                }
                                screenshots.emplace_back(f.getFileName(), bm);
                            }
                        } catch (image::image_exception& e) {
                            LOGD("Failed to load image");
                        }
                    }

                    if (screenshots.size() >= t) {
                        screenshots.erase(std::remove(screenshots.begin(),
                                                      screenshots.end(), ""),
                                          screenshots.end());
                        sort(screenshots.begin(), screenshots.end());
                        nextScreenshot();
                    }
                };
                for (auto& p : parts)
                    webutils::Web::getInstance().getFile(p, cb);
            }
        } else
            nextScreenshot();

        // Make sure any previous tween is complete
        currentTween.finish();
        currentInfoField[0].pos.x = currentInfoField[1].pos.x;
        prevInfoField = currentInfoField;

        // Update current info, rely on tween to make sure it will fade in
        currentInfoField.setInfo(currentInfo);
        // currentTune = currentInfo.starttune;
        currentTune = player.getTune();

        if (currentInfo.numtunes > 0)
            songField.setText(utils::format("[%02d/%02d]", currentTune + 1,
                                            currentInfo.numtunes));
        else
            songField.setText("[01/01]");

        auto sub_title = player.getMeta("sub_title");

        int tw = currentInfoField.getWidth(0);

        auto f = [=]() {
            xinfoField.setText(sub_title);
            int d = (tw - (downRight.x - topLeft.x - 20));
            if (d > 20)
                Tween::make()
                    .sine()
                    .repeating()
                    .to(currentInfoField[0].pos.x,
                        currentInfoField[0].pos.x - d)
                    .seconds((d + 200) / 200.0f);
        };

        updateFavorite();
        // Start tweening
        updateNextField();
        player.playlistUpdated();
        LOGD("## TWEENING INFO FIELDS");

        if (player.wasFromQueue()) {
            currentTween = Tween::make() // target , source  <------
                               .from(prevInfoField, currentInfoField)
                               .from(currentInfoField, nextInfoField)
                               .from(nextInfoField, outsideInfoField)
                               .seconds(1.5)
                               .onComplete(f);
        } else {
            currentTween = Tween::make()
                               .from(prevInfoField, currentInfoField)
                               .from(currentInfoField, outsideInfoField)
                               .seconds(1.5)
                               .onComplete(f);
        }
        currentTween.start();
    }

    if (playerState == MusicPlayerList::Error) {
        player.stop();
        currentTween.finish();
        currentInfoField[0].pos.x = currentInfoField[1].pos.x;

        SongInfo song = player.getInfo();
        prevInfoField.setInfo(song);
        LOGD("SONG %s could not be played", song.path);
        currentTween = Tween::make()
                           .from(prevInfoField, nextInfoField)
                           .seconds(3.0)
                           .onComplete([=]() {
                               if (playerState == MusicPlayerList::Stopped)
                                   player.nextSong();
                           });
        currentTween.start();
    }

    if (playerState == MusicPlayerList::Playing ||
        playerState == MusicPlayerList::Stopped) {

        if (player.playlistUpdated()) {
            updateNextField();
        }
    }

    int tune = player.getTune();
    if (currentTune != tune) {
        songField.add = 0.0;
        Tween::make().sine().to(songField.add, 1.0).seconds(0.5);
        currentInfo = player.getInfo();
        auto sub_title = player.getMeta("sub_title");
        xinfoField.setText(sub_title);
        currentInfoField.setInfo(currentInfo);
        currentTune = tune;
        songField.setText(utils::format("[%02d/%02d]", currentTune + 1,
                                        currentInfo.numtunes));
        auto m = compressWhitespace(player.getMeta("message"));
        if (m != "" && scrollText != m) {
            scrollEffect.set("scrolltext", m);
            scrollText = m;
        }
        updateFavorite();
    }

    if (player.isPlaying()) {

        auto br = player.getBitrate();
        if (br > 0) {
            songField.setText(utils::format("%d KBit", br));
        }

        auto p = player.getPosition();
        int length = player.getLength();
        timeField.setText(utils::format("%02d:%02d", p / 60, p % 60));
        if (length > 0)
            lengthField.setText(
                utils::format("(%02d:%02d)", length / 60, length % 60));
        else
            lengthField.setText("");

        auto sub_title = player.getMeta("sub_title");
        if (sub_title != xinfoField.getText()) xinfoField.setText(sub_title);

#ifdef DO_WE_NEED_THIS
        if (scrollText == "") {
            auto m = player.getMeta("message");
            if (m != "") {
                m = compressWhitespace(m);
                if (scrollText != m) {
                    scrollEffect.set("scrolltext", m);
                    scrollText = m;
                }
            }
        }
#endif
    }

    if (player.hasError()) {
        toast(player.getError(), ERROR);
    }

    if (!player.isPaused()) {
        for (auto& e : eq) {
            if (e >= 4 * 4)
                e -= 2 * 4;
            else
                e = 2 * 4;
        }
    }

    if (player.isPlaying()) {
        auto delay = 1; // AudioPlayer::get_delay();
        if (fft.size() > delay) {
            while (fft.size() > delay + 4) {
                fft.popLevels();
            }
            spectrum = fft.getLevels();
            fft.popLevels();
        }
        for (auto i : utils::count_to(fft.eq_slots)) {
            if (spectrum[i] > 5) {
                auto f = static_cast<unsigned>(logf(spectrum[i]) * 64);
                if (f > 255) f = 255;
                if (f > eq[i]) eq[i] = static_cast<uint8_t>(f);
            }
        }
    }
    bool busy = (
#ifdef ENABLE_TELNET
        WebRPC::inProgress() > 0 ||
#endif
        playerState == MusicPlayerList::Loading ||
        webutils::Web::inProgress() > 0);

    netIcon.visible(busy);

    if (setShotAt < utils::getms() - 10000) nextScreenshot();
}

void fadeOut(float& alpha, float t = 0.25)
{
    Tween::make().to(alpha, 0.0).seconds(t);
}

void ChipMachine::toast(std::string const& txt, ToastType type)
{

    static std::vector<Color> colors = {
        0xffffff, 0xff8888, 0x55aa55
    }; // Alpha intentionally left at zero

    toastField.setText(txt);
    int tlen = toastField.getWidth();
    toastField.pos.x = topLeft.x + ((downRight.x - topLeft.x) - tlen) / 2;
    toastField.color = colors[(int)type % 3];

    Tween::make()
        .to(toastField.color.alpha, 1.0)
        .seconds(0.25)
        .onComplete([=]() {
            if ((int)type < 3)
                Tween::make()
                    .to(toastField.color.alpha, 0.0)
                    .delay(1.0)
                    .seconds(0.25);
        });
}

void ChipMachine::removeToast()
{
    toastField.setText("");
    toastField.color = 0;
}

void ChipMachine::render(uint32_t delta)
{

    if (screen.size() != screenSize) {
        resizeDelay = 2;
        screenSize = screen.size();
    }

    if (resizeDelay) {
        resizeDelay--;
        if (resizeDelay == 0) {
            layoutScreen();
        }
    }

    screen.clear(0xff000000 | bgcolor);

    if (showVolume) {
        static Color color = 0xff000000;
        showVolume--;

        volumeIcon.render(screenptr, 0);
        auto v = (int)(player.getVolume() * 10);
        v = (int)(v * volPos.w) / 10;
        screen.rectangle(volPos.x + v, volPos.y, volPos.w - v, volPos.h, color);
        // screen.text(listFont, std::to_std::string((int)(v * 100)), volPos.x,
        // volPos.y, 10.0, 0xff8888ff);
    }

    musicBars.render(spectrumPos, spectrumColor, eq);

    if (starsOn) starEffect.render(delta);
    scrollEffect.render(delta);

    if (currentScreen == MAIN_SCREEN) {
        mainScreen.render(screenptr, delta);
    } else if (currentScreen == SEARCH_SCREEN) {
        searchScreen.render(screenptr, delta);
    } else {
        commandScreen.render(screenptr, delta);
    }

    overlay.render(screenptr, delta);

    font.update_cache();
    listFont.update_cache();

    screen.flip();

    webutils::Web::pollAll();
}
} // namespace chipmachine

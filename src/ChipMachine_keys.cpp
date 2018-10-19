#include "ChipMachine.h"
#include "modutils.h"
#include <algorithm>
#include <random>
#include <chrono>

using tween::Tween;

namespace chipmachine {

// see also: https://github.com/Attnam/ivan/pull/407
static std::mt19937 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());

void ChipMachine::addKey(uint32_t key, statemachine::Condition const& cond,
                         std::string const& cmd)
{

    auto screen = currentScreen;
    bool onMain = false;
    bool onSearch = false;

    currentScreen = NO_SCREEN;
    if (!cond.check()) {
        currentScreen = MAIN_SCREEN;
        onMain = cond.check();
        currentScreen = SEARCH_SCREEN;
        onSearch = cond.check();
    }
    currentScreen = screen;

    auto it = std::find(commands.begin(), commands.end(), cmd);
    if (it != commands.end()) {
        smac.add(key, cond,
                 static_cast<uint32_t>(std::distance(commands.begin(), it)));
        if (key == keycodes::BACKSPACE)
            return;
        if (it->shortcut == "") {
            std::string name;
            if (key & SHIFT)
                name += "shift+";
            if (key & ALT)
                name += "alt+";
            if (key & CTRL)
                name += "ctrl+";
            key &= 0xffff;
            if (key < 0x100)
                name.append(1, tolower(key));
            else if (key <= keycodes::F12)
                name += utils::toLower(key_names[key - 0x100]);
            if (onSearch)
                name += " [search]";
            if (onMain)
                name += " [main]";
            it->shortcut = name;
        }
    }
}

void ChipMachine::setupRules()
{

    using namespace statemachine;

    addKey(keycodes::F1, "show_main");
    addKey(keycodes::F2, "show_search");
    addKey({keycodes::UP, keycodes::DOWN, keycodes::PAGEUP, keycodes::PAGEDOWN},
           if_equals(currentScreen, MAIN_SCREEN), "show_search");
    addKey(keycodes::F5, "play_pause");
    addKey(keycodes::F3, "show_command");

    addKey(keycodes::BACKSPACE,
           if_equals(currentScreen, SEARCH_SCREEN) && if_null(currentDialog) &&
               if_false(haveSearchChars),
           "clear_filter");

    addKey(keycodes::ESCAPE, if_not_null(currentDialog), "close_dialog");
    addKey(keycodes::ESCAPE, if_equals(currentScreen, COMMAND_SCREEN),
           "clear_command");
    addKey(keycodes::ESCAPE, if_equals(currentScreen, SEARCH_SCREEN),
           "clear_search");

    addKey(keycodes::F6, "next_song");
    addKey(keycodes::ENTER, if_equals(currentScreen, MAIN_SCREEN), "next_song");
    addKey(keycodes::ENTER, if_equals(currentScreen, SEARCH_SCREEN),
           "play_song");
    addKey(keycodes::ENTER, if_equals(currentScreen, COMMAND_SCREEN),
           "execute_selected_command");
    addKey(keycodes::ENTER | SHIFT, if_equals(currentScreen, SEARCH_SCREEN),
           "enque_song");
    addKey(keycodes::F9, if_equals(currentScreen, SEARCH_SCREEN), "enque_song");
    addKey(keycodes::DOWN | SHIFT, if_equals(currentScreen, SEARCH_SCREEN),
           "next_composer");
    addKey(keycodes::F7, if_equals(currentScreen, SEARCH_SCREEN),
           "add_list_favorite");
    addKey(keycodes::F7, if_equals(currentScreen, MAIN_SCREEN),
           "add_current_favorite");
    addKey(keycodes::F8, "clear_songs");
    addKey(keycodes::LEFT,
           if_not_equals(currentScreen, COMMAND_SCREEN) &&
               if_null(currentDialog),
           "prev_subtune");
    addKey(keycodes::RIGHT,
           if_not_equals(currentScreen, COMMAND_SCREEN) &&
               if_null(currentDialog),
           "next_subtune");
    addKey(keycodes::F4, "layout_screen");
    addKey(keycodes::ESCAPE | SHIFT, "quit");
    addKey(keycodes::F4 | ALT, "quit");

    addKey('d' | CTRL, "download_current");
    addKey('z' | CTRL, "next_screenshot");

    addKey('r' | CTRL, "random_shuffle");
    addKey('f' | CTRL, "format_shuffle");
    addKey('c' | CTRL, "composer_shuffle");
    addKey('s' | CTRL, "result_shuffle");
    addKey('o' | CTRL, "collection_shuffle");
    addKey('g' | CTRL, "favorite_shuffle");
    addKey('-', "volume_down");
    addKey({'+', '='}, "volume_up");
    addKey(keycodes::TAB, "toggle_command");
    std::string empty("");
    addKey('i' | CTRL, if_equals(filter, empty), "set_collection_filter");
    addKey('i' | CTRL, if_not_equals(filter, empty), "clear_filter");
}

void ChipMachine::showScreen(Screen screen)
{
    if (currentScreen != screen) {
        hasMoved = (screen != SEARCH_SCREEN);
        currentScreen = screen;
        if (screen == MAIN_SCREEN) {
            Tween::make().to(spectrumColor, spectrumColorMain).seconds(0.5);
            Tween::make().to(scrollEffect.alpha, 1.0).seconds(0.5);
        } else {
            Tween::make().to(spectrumColor, spectrumColorSearch).seconds(0.5);
            Tween::make().to(scrollEffect.alpha, 0.0).seconds(0.5);
        }
    }
}

SongInfo ChipMachine::getSelectedSong()
{
    int i = songList.selected();
    if (i < 0)
        return SongInfo();
    return MusicDatabase::getInstance().getSongInfo(iquery->getIndex(i));
}

void ChipMachine::shuffleFavorites()
{
    std::vector<SongInfo> target =
        MusicDatabase::getInstance().getPlaylist(currentPlaylistName);
    std::shuffle(target.begin(), target.end(), rng);
    playSongs(target);
}

void ChipMachine::shuffleSongs(int what, int limit)
{
    std::vector<SongInfo> target;
    SongInfo match =
        (currentScreen == SEARCH_SCREEN) ? getSelectedSong() : dbInfo;

    LOGD("SHUFFLE %s / %s", match.composer, match.format);

    if (!(what & Shuffle::Format))
        match.format = "";
    if (!(what & Shuffle::Composer))
        match.composer = "";
    if (!(what & Shuffle::Collection))
        match.path = "";
    match.title = match.game;

    MusicDatabase::getInstance().getSongs(target, match, limit, true);
    playSongs(target);
}

void ChipMachine::playSongs(std::vector<SongInfo> const& songs)
{
    player.clearSongs();
    for (const auto& s : songs) {
        if (!utils::endsWith(s.path, ".plist"))
            player.addSong(s);
    }
    showScreen(MAIN_SCREEN);
    player.nextSong();
}

void ChipMachine::updateKeys()
{

    using namespace grappix;

    haveSearchChars = (iquery->getString().length() > 0);

    searchUpdated = false;
    auto last_selection = songList.selected();

    auto key = screen.get_key();

    if ((key & 0x80000000) != 0)
        return;

    // LOGD("KEY %x", key);

    if (indexingDatabase)
        return;

    uint32_t event = key;

    VerticalList* currentList = nullptr;
    if (currentScreen == SEARCH_SCREEN)
        currentList = &songList;
    else if (currentScreen == COMMAND_SCREEN)
        currentList = &commandList;

    bool ascii = (event >= 'A' && event <= 'Z');
    if (ascii)
        event = tolower(event);
    if (screen.key_pressed(keycodes::SHIFT_LEFT) ||
        screen.key_pressed(keycodes::SHIFT_RIGHT)) {
        if (ascii)
            event = toupper(event);
        else if (event == keycodes::DOWN)
            key = keycodes::UP;
        else
            event |= SHIFT;
    }

    if (screen.key_pressed(keycodes::CTRL_LEFT) ||
        screen.key_pressed(keycodes::CTRL_RIGHT)) {
        if (event == keycodes::DOWN)
            key = keycodes::PAGEDOWN;
        else if (event == keycodes::UP)
            key = keycodes::PAGEUP;
        else
            event |= CTRL;
    }
    if (screen.key_pressed(keycodes::ALT_LEFT) ||
        screen.key_pressed(keycodes::ALT_RIGHT))
        event |= ALT;

    if ((event & (CTRL | SHIFT)) == 0 && currentList)
        currentList->onKey(key);

    if (event == (keycodes::RIGHT | SHIFT))
        event = keycodes::LEFT;

    lastKey = key;

    if (!smac.put_event(event)) {
        if ((key >= ' ' && key <= 'z') || key == keycodes::LEFT ||
            key == keycodes::RIGHT || key == keycodes::BACKSPACE ||
            key == keycodes::ESCAPE || key == keycodes::ENTER) {
            if (currentDialog != nullptr) {
                currentDialog->on_key(event);
            } else if (currentScreen == COMMAND_SCREEN) {
                commandField.on_key(event);
                auto ctext = commandField.getText();
                if (ctext == "")
                    clearCommand();
                else {
                    matchingCommands.resize(commands.size());
                    int j = 0;
                    for (int i = 0; i < commands.size(); i++) {
                        if (utils::toLower(commands[i].name).find(ctext) !=
                            std::string::npos)
                            matchingCommands[j++] = &commands[i];
                    }
                    matchingCommands.resize(j);
                }
                commandList.setTotal(matchingCommands.size());
            } else {
                if (hasMoved && event != ' ' && event != keycodes::BACKSPACE)
                    searchField.setText("");
                hasMoved = false;
                showScreen(SEARCH_SCREEN);
                if (event >= 0x20 && event <= 0xff)
                    event = tolower(event);
                searchField.on_key(event);
                searchUpdated = true;
            }
        }
    }
    while (smac.actionsLeft() > 0) {
        auto action = smac.next_action();
        commands[action.id].fn();
    }

    if (songList.selected() != last_selection && iquery->numHits() > 0) {
        int i = songList.selected();
        SongInfo song =
            MusicDatabase::getInstance().getSongInfo(iquery->getIndex(i));
        auto ext = getTypeFromName(song.path);
        bool isoffline = RemoteLoader::getInstance().isOffline(song.path);
        if (ext != "")
            topStatus.setText(utils::format("Format: %s (%s)%s", song.format,
                                            ext, isoffline ? "*" : ""));
        else
            topStatus.setText(utils::format("Format: %s %s", song.format,
                                            isoffline ? "*" : ""));
        searchField.visible(false);
        filterField.visible(false);
        topStatus.visible(true);
    }

    if (searchUpdated) {
        auto s = searchField.getText();
        if (s[0] == '\\') {
            int pos = s.find(' ');
            if (pos != std::string::npos) {
                auto f = s.substr(1, pos - 1);
                if (f != filter) {
                    filter = f;
                    s = s.substr(pos + 1);
                    searchField.setText(s);
                }
            }
        }

        if (filter != filterField.getText()) {
            LOGD("Filter now %s", filter);
            filterField.setText(filter);
            MusicDatabase::getInstance().setFilter(filter);
            iquery->invalidate();
        }

        iquery->setString(s);
        searchField.visible(true);
        filterField.visible(true);
        searchField.pos.x = filterField.pos.x + filterField.getWidth() + 5;
        topStatus.visible(false);
        songList.setTotal(iquery->numHits());
        searchUpdated = false;
    }
}

} // namespace chipmachine

#include "ui.h"
#include "player.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <thread>

using namespace ftxui;

namespace {

std::string format_time(float seconds) {
    if (seconds < 0 || !std::isfinite(seconds)) seconds = 0;
    int total = static_cast<int>(seconds);
    int m = total / 60, s = total % 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return buf;
}

} // namespace

void run_ui(std::vector<std::filesystem::path>& songs) {
    Player player;

    std::vector<std::string> names;
    names.reserve(songs.size());
    for (const auto& p : songs)
        names.push_back(p.filename().string());
    
    std::vector<std::string> path_strings;
    path_strings.reserve(songs.size());
    for (const auto& p : songs) path_strings.push_back(p.string());

    player.set_paths(&path_strings);
    player.set_track_count(path_strings.size());

    int selected = 0;
    auto menu = Menu(&names, &selected);

    std::string now_playing = "nothing playing";

    auto screen = ScreenInteractive::Fullscreen();

    // --- refresh thread: forces redraw ~2x/sec so the bar ticks ---
    std::atomic<bool> quit{false};
    std::thread refresher([&] {
        while (!quit) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (player.finished()) {
                player.next();
                int idx = player.current_index();
                if (idx >= 0) now_playing = names[idx];
            }
            screen.Post(Event::Custom);
        }
    });

    auto component = Renderer(menu, [&] {
        float len = player.length_seconds();
        float cur = player.cursor_seconds();
        float progress = (len > 0.0f) ? cur / len : 0.0f;

        return vbox({
            text(" music-cpp  [" + std::to_string(songs.size()) + " tracks]") | bold | inverted,
            separator(),
            menu->Render() | vscroll_indicator | frame | flex,
            separator(),
            hbox({
                text(" " + now_playing) | flex,
                text(format_time(cur) + " / " + format_time(len) + " "),
            }),
            gauge(progress) | color(Color::Green),
            hbox({
                text(player.is_paused() ? " || " : " > "),
                text(now_playing) | flex,
                text(player.shuffle() ? " [shuffle]" : "") | color(Color::Yellow),
                text(player.repeat() == RepeatMode::All  ? " [repeat-all]" :
                     player.repeat() == RepeatMode::One  ? " [repeat-one]" : "") | color(Color::Yellow),
                text(" " + format_time(cur) + " / " + format_time(len) + " "),
            }),
        });
    });

    component = CatchEvent(component, [&](Event event) {
        if (event == Event::Character('q')) { screen.Exit(); quit=true; return true; }

        if (event == Event::Character('j')) {
            selected = std::min(selected + 1, (int)songs.size() - 1);
            return true;
        }
        if (event == Event::Character('k')) {
            selected = std::max(selected - 1, 0);
            return true;
        }

        if (event == Event::Return) {
            if (player.play_index(selected))
                now_playing = names[selected];
            else
                now_playing = "x failed: " + names[selected];
            return true;
        }

        if (event == Event::Character(' ')) { player.toggle_pause(); return true; }
        if (event == Event::Character('h')) { player.seek_seconds(player.cursor_seconds() - 5); return true; }
        if (event == Event::Character('l')) { player.seek_seconds(player.cursor_seconds() + 5); return true; }
        if (event == Event::Character('-')) { player.set_volume(player.volume() - 0.05f); return true; }
        if (event == Event::Character('=')) { player.set_volume(player.volume() + 0.05f); return true; }
        if (event == Event::Character('n')) { player.next(); return true; }
        if (event == Event::Character('p')) { player.previous(); return true; }
        if (event == Event::Character('s')) { player.toggle_shuffle(); return true; }
        if (event == Event::Character('r')) { player.cycle_repeat(); return true; }

        return false;
    });

    screen.Loop(component);

    quit = true;
    refresher.join();
}

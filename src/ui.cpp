#include "ui.h"
#include "player.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>
#include <thread>

using namespace ftxui;

namespace {

std::string format_time(float seconds) {
    if (seconds < 0 || !std::isfinite(seconds)) seconds = 0;
    int total = static_cast<int>(seconds);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d:%02d", total / 60, total % 60);
    return buf;
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

} // namespace

void run_ui(std::vector<std::filesystem::path>& songs) {
    Player player;

    std::vector<std::string> names;
    std::vector<std::string> path_strings;
    names.reserve(songs.size());
    path_strings.reserve(songs.size());
    for (const auto& p : songs) {
        names.push_back(p.filename().string());
        path_strings.push_back(p.string());
    }

    player.set_paths(&path_strings);
    player.set_track_count(path_strings.size());

    // ---- search state ----
    std::string query;
    bool search_mode = false;
    auto input = Input(&query, "type to filter...");

    // filtered view: filtered[i] = real song index shown at row i
    std::vector<int> filtered;
    std::vector<std::string> filtered_names;
    int selected = 0;

    auto apply_filter = [&] {
        std::string q = to_lower(query);
        filtered.clear();
        filtered_names.clear();
        for (int i = 0; i < (int)songs.size(); ++i) {
            if (q.empty() || to_lower(names[i]).find(q) != std::string::npos) {
                filtered.push_back(i);
                filtered_names.push_back(names[i]);
            }
        }
        if (filtered.empty()) selected = 0;
        else selected = std::clamp(selected, 0, (int)filtered.size() - 1);
    };
    apply_filter();

    auto menu = Menu(&filtered_names, &selected);

    auto screen = ScreenInteractive::Fullscreen();
    std::atomic<bool> quit{false};
    std::thread refresher([&] {
        while (!quit) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (player.finished())
                player.next(true);
            screen.Post(Event::Custom);
        }
    });

    // menu + input stacked; input only rendered in search mode
    auto container = Container::Vertical({menu, input});

    auto component = Renderer(container, [&] {
        float len = player.length_seconds();
        float cur = player.cursor_seconds();
        float progress = (len > 0.0f) ? cur / len : 0.0f;

        Elements bottom;
        if (search_mode) {
            bottom.push_back(hbox({text(" /"), input->Render()}) | color(Color::Cyan));
        } else if (!query.empty()) {
            bottom.push_back(
                text(" filter: " + query + "  (" + std::to_string(filtered.size()) + " matches, Esc to clear)") | dim);
        }
        bottom.push_back(hbox({
            text(player.current_index() < 0 ? " nothing playing"
                 : (player.is_paused() ? " || " : " > ") + names[player.current_index()]) | flex,
            text(player.shuffle() ? " [shuffle]" : "") | color(Color::Yellow),
            text(player.repeat() == RepeatMode::All ? " [repeat-all]"
                 : player.repeat() == RepeatMode::One ? " [repeat-one]" : "") | color(Color::Yellow),
            text(" " + format_time(cur) + " / " + format_time(len) + " "),
        }));
        bottom.push_back(gauge(progress) | color(Color::Green));
        bottom.push_back(hbox({
            text(" vol ") | dim,
            gauge(player.volume()) | size(WIDTH, EQUAL, 12) | dim,
            text(" " + std::to_string(int(player.volume() * 100)) + "% ") | dim,
        }));

        return vbox({
            text(" music-cpp  [" + std::to_string(filtered.size()) + "/" +
                 std::to_string(songs.size()) + " tracks]") | bold | inverted,
            separator(),
            menu->Render() | vscroll_indicator | frame | flex,
            separator(),
            vbox(std::move(bottom)),
        });
    });

    component = CatchEvent(component, [&](Event event) {
        // ---- search mode: keys go to the input box ----
        if (search_mode) {
            if (event == Event::Escape) {          // cancel: clear filter, back to list
                query.clear();
                apply_filter();
                search_mode = false;
                menu->TakeFocus();
                return true;
            }
            if (event == Event::Return) {          // accept: play selected match
                if (!filtered.empty())
                    player.play_index(filtered[selected]);
                search_mode = false;
                menu->TakeFocus();
                return true;
            }
            bool handled = input->OnEvent(event);  // letters, backspace, arrows...
            apply_filter();                        // re-filter on every keystroke
            return true;
        }

        // ---- normal mode ----
        if (event == Event::Character('/')) {
            search_mode = true;
            input->TakeFocus();
            return true;
        }
        if (event == Event::Escape) {              // clear lingering filter without typing /
            if (!query.empty()) { query.clear(); apply_filter(); }
            return true;
        }
        if (event == Event::Character('q')) { screen.Exit(); return true; }

        if (event == Event::Character('j')) {
            if (!filtered.empty()) selected = std::min(selected + 1, (int)filtered.size() - 1);
            return true;
        }
        if (event == Event::Character('k')) {
            selected = std::max(selected - 1, 0);
            return true;
        }

        if (event == Event::Return) {
            if (!filtered.empty())
                player.play_index(filtered[selected]);
            return true;
        }

        if (event == Event::Character('n')) { player.next(); return true; }
        if (event == Event::Character('p')) { player.previous(); return true; }
        if (event == Event::Character('s')) { player.toggle_shuffle(); return true; }
        if (event == Event::Character('r')) { player.cycle_repeat(); return true; }
        if (event == Event::Character(' ')) { player.toggle_pause(); return true; }
        if (event == Event::Character('h')) { player.seek_seconds(player.cursor_seconds() - 5); return true; }
        if (event == Event::Character('l')) { player.seek_seconds(player.cursor_seconds() + 5); return true; }
        if (event == Event::Character('-')) { player.set_volume(player.volume() - 0.05f); return true; }
        if (event == Event::Character('=')) { player.set_volume(player.volume() + 0.05f); return true; }

        return false;
    });

    screen.Loop(component);

    quit = true;
    refresher.join();
}

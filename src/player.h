#pragma once

#include <string>
#include <vector>

struct ma_engine;
struct ma_sound;

enum class RepeatMode { Off, All, One };

class Player {
public:
    Player();
    ~Player();

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    void set_track_count(size_t n);   // call once after scan

    bool play_index(int idx);         // play song #idx (path set via set_paths)
    void set_paths(const std::vector<std::string>* paths) { paths_ = paths; }

    
    void next(bool automatic = false);
    void previous();
    void toggle_shuffle();
    void cycle_repeat();              // Off -> All -> One -> Off

    bool shuffle() const { return shuffle_; }
    RepeatMode repeat() const { return repeat_; }
    int current_index() const { return current_; }

    void toggle_pause();
    bool is_paused() const { return paused_; }
    bool finished() const;            // track ended naturally -> auto-advance
    bool is_playing() const;

    float length_seconds() const;
    float cursor_seconds() const;
    void seek_seconds(float t);

    void set_volume(float v);
    float volume() const { return volume_; }

private:
    void build_order(int keep_current = -1);
    void start_current();

    ma_engine* engine_ = nullptr;
    ma_sound* sound_ = nullptr;

    const std::vector<std::string>* paths_ = nullptr;

    std::vector<int> order_;          // playback order = indices into *paths_
    int order_pos_ = 0;               // position inside order_
    int current_ = -1;                // actual song index, -1 = none

    bool shuffle_ = false;
    RepeatMode repeat_ = RepeatMode::Off;
    bool paused_ = false;
    float volume_ = 1.0f;
};

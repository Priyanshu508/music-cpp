#include "player.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <algorithm>
#include <numeric>
#include <random>

Player::Player() {
    engine_ = new ma_engine;
    if (ma_engine_init(nullptr, engine_) != MA_SUCCESS) {
        delete engine_;
        engine_ = nullptr;
    }
}

Player::~Player() {
    if (sound_) { ma_sound_uninit(sound_); delete sound_; }
    if (engine_) { ma_engine_uninit(engine_); delete engine_; }
}

void Player::set_track_count(size_t n) {
    order_.resize(n);
    std::iota(order_.begin(), order_.end(), 0);
}

void Player::start_current() {
    if (sound_) { ma_sound_uninit(sound_); delete sound_; sound_ = nullptr; }

    sound_ = new ma_sound;
    if (ma_sound_init_from_file(engine_, (*paths_)[current_].c_str(),
                                MA_SOUND_FLAG_STREAM, nullptr, nullptr,
                                sound_) != MA_SUCCESS) {
        delete sound_;
        sound_ = nullptr;
        return;
    }
    ma_sound_set_volume(sound_, volume_);
    ma_sound_start(sound_);
    paused_ = false;
}

bool Player::play_index(int idx) {
    if (!engine_ || !paths_ || idx < 0 || idx >= (int)paths_->size()) return false;

    current_ = idx;
    // rebuild order so "next" from here follows the displayed list
    // (unless shuffle is on — then rebuild a fresh shuffled order)
    build_order(idx);
    start_current();
    return sound_ != nullptr;
}

void Player::build_order(int keep_current) {
    if (!paths_) return;
    order_.resize(paths_->size());
    std::iota(order_.begin(), order_.end(), 0);

    if (shuffle_) {
        std::shuffle(order_.begin(), order_.end(), std::mt19937{std::random_device{}()});
    }

    // keep the playing track at the front of the remaining order
    if (keep_current >= 0) {
        auto it = std::find(order_.begin(), order_.end(), keep_current);
        if (it != order_.end()) std::iter_swap(order_.begin(), it);
        order_pos_ = 0;
    }
}

void Player::next() {
    if (order_.empty() || current_ < 0) return;

    if (repeat_ == RepeatMode::One) { start_current(); return; }

    ++order_pos_;
    if (order_pos_ >= (int)order_.size()) {
        if (repeat_ == RepeatMode::All) order_pos_ = 0;
        else return;  // end of queue, stay stopped
    }
    current_ = order_[order_pos_];
    start_current();
}

void Player::previous() {
    if (order_.empty() || current_ < 0) return;

    // cmus behavior: if >3s into the track, restart it instead of going back
    if (cursor_seconds() > 3.0f) { seek_seconds(0.0f); return; }

    --order_pos_;
    if (order_pos_ < 0) {
        if (repeat_ == RepeatMode::All) order_pos_ = (int)order_.size() - 1;
        else { order_pos_ = 0; seek_seconds(0.0f); return; }
    }
    current_ = order_[order_pos_];
    start_current();
}

void Player::toggle_shuffle() {
    shuffle_ = !shuffle_;
    build_order(current_);
}

void Player::cycle_repeat() {
    switch (repeat_) {
        case RepeatMode::Off: repeat_ = RepeatMode::All; break;
        case RepeatMode::All: repeat_ = RepeatMode::One; break;
        case RepeatMode::One: repeat_ = RepeatMode::Off; break;
    }
}

void Player::toggle_pause() {
    if (!sound_) return;
    if (paused_) { ma_sound_start(sound_); paused_ = false; }
    else         { ma_sound_stop(sound_);  paused_ = true;  }
}

bool Player::is_playing() const {
    return sound_ && ma_sound_is_playing(sound_);
}

bool Player::finished() const {
    // loaded, not paused by user, and engine says it's not playing -> it ended
    return sound_ && !paused_ && !ma_sound_is_playing(sound_);
}

float Player::length_seconds() const {
    if (!sound_) return 0.0f;
    float len = 0.0f;
    ma_sound_get_length_in_seconds(sound_, &len);
    return len;
}

float Player::cursor_seconds() const {
    if (!sound_) return 0.0f;
    float cur = 0.0f;
    ma_sound_get_cursor_in_seconds(sound_, &cur);
    return cur;
}

void Player::seek_seconds(float t) {
    if (!sound_) return;
    t = std::clamp(t, 0.0f, length_seconds());
    ma_sound_seek_to_second(sound_, t);
}

void Player::set_volume(float v) {
    volume_ = std::clamp(v, 0.0f, 1.0f);
    if (sound_) ma_sound_set_volume(sound_, volume_);
}

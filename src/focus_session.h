#pragma once

#include <chrono>

class FocusSession {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    enum class State { Idle, Running, Paused, Completed };
    enum class Event { None, GentleReminder, Completed };

    // requestedMinutes is 1..999, deltaMinutes is -10..10, reminderFraction is [0.65, 0.75).
    void Start(int requestedMinutes, int deltaMinutes, double reminderFraction, TimePoint now);
    Event Update(TimePoint now);
    void Pause(TimePoint now);
    void Resume(TimePoint now);
    void Cancel();

    State GetState() const { return state_; }
    int ActualMinutes() const { return actualMinutes_; }

private:
    State state_ = State::Idle;
    int actualMinutes_ = 0;
    double reminderFraction_ = 0.0;
    bool reminderPlayed_ = false;
    Clock::duration elapsed_{};
    TimePoint runningSince_{};
};

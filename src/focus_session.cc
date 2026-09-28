#include "focus_session.h"

#include <algorithm>
#include <stdexcept>

void FocusSession::Start(int requestedMinutes, int deltaMinutes, double reminderFraction, TimePoint now)
{
    if (requestedMinutes < 1 || requestedMinutes > 999 || deltaMinutes < -10 ||
        deltaMinutes > 10 || !(reminderFraction >= 0.65 && reminderFraction < 0.75)) {
        throw std::invalid_argument("Invalid focus session parameters");
    }

    actualMinutes_ = std::max(1, requestedMinutes + deltaMinutes);
    reminderFraction_ = reminderFraction;
    reminderPlayed_ = false;
    elapsed_ = Clock::duration::zero();
    runningSince_ = now;
    state_ = State::Running;
}

FocusSession::Event FocusSession::Update(TimePoint now)
{
    if (state_ != State::Running) return Event::None;

    const auto active = elapsed_ + std::max(Clock::duration::zero(), now - runningSince_);
    const double seconds = std::chrono::duration<double>(active).count();
    const double totalSeconds = static_cast<double>(actualMinutes_) * 60.0;

    if (seconds >= totalSeconds) {
        elapsed_ = active;
        state_ = State::Completed;
        return Event::Completed;
    }
    if (!reminderPlayed_ && seconds >= totalSeconds * reminderFraction_) {
        reminderPlayed_ = true;
        return Event::GentleReminder;
    }
    return Event::None;
}

void FocusSession::Pause(TimePoint now)
{
    if (state_ != State::Running) return;
    elapsed_ += std::max(Clock::duration::zero(), now - runningSince_);
    state_ = State::Paused;
}

void FocusSession::Resume(TimePoint now)
{
    if (state_ != State::Paused) return;
    runningSince_ = now;
    state_ = State::Running;
}

void FocusSession::Cancel()
{
    state_ = State::Idle;
    actualMinutes_ = 0;
    elapsed_ = Clock::duration::zero();
    reminderPlayed_ = false;
}

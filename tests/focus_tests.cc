#include "focus_session.h"
#include "progress_store.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

namespace {
using Clock = FocusSession::Clock;
using namespace std::chrono_literals;

void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

void TestDurationAndRandomBounds()
{
    const auto zero = Clock::time_point{};
    FocusSession session;
    session.Start(1, -10, 0.65, zero);
    Check(session.ActualMinutes() == 1, "duration must be clamped to one minute");
    session.Start(999, 10, 0.74999, zero);
    Check(session.ActualMinutes() == 1009, "maximum offset should apply");

    std::mt19937 generator(7);
    std::uniform_int_distribution<int> delta(-10, 10);
    std::uniform_real_distribution<double> ratio(0.65, 0.75);
    for (int i = 0; i < 10000; ++i) {
        const int drawnDelta = delta(generator);
        const double drawnRatio = ratio(generator);
        Check(drawnDelta >= -10 && drawnDelta <= 10, "delta outside bounds");
        Check(drawnRatio >= 0.65 && drawnRatio < 0.75, "ratio outside bounds");
        session.Start(1, drawnDelta, drawnRatio, zero);
        Check(session.ActualMinutes() >= 1, "drawn duration must be positive");
    }
}

void TestReminderPauseAndCompletion()
{
    const auto zero = Clock::time_point{};
    FocusSession session;
    session.Start(1, 0, 0.70, zero);
    Check(session.Update(zero + 41s) == FocusSession::Event::None, "reminder too early");
    Check(session.Update(zero + 42s) == FocusSession::Event::GentleReminder, "reminder missing");
    Check(session.Update(zero + 43s) == FocusSession::Event::None, "reminder repeated");
    Check(session.Update(zero + 60s) == FocusSession::Event::Completed, "completion missing");
    Check(session.Update(zero + 61s) == FocusSession::Event::None, "completion repeated");

    session.Start(1, 0, 0.70, zero);
    session.Pause(zero + 30s);
    Check(session.Update(zero + 200s) == FocusSession::Event::None, "paused session advanced");
    session.Resume(zero + 200s);
    Check(session.Update(zero + 211s) == FocusSession::Event::None, "paused time was counted");
    Check(session.Update(zero + 212s) == FocusSession::Event::GentleReminder, "resumed reminder missing");
    Check(session.Update(zero + 230s) == FocusSession::Event::Completed, "resumed completion missing");
}

void TestCancelAndSimultaneousThresholds()
{
    const auto zero = Clock::time_point{};
    FocusSession session;
    session.Start(1, 0, 0.70, zero);
    session.Cancel();
    Check(session.Update(zero + 100s) == FocusSession::Event::None, "cancelled session completed");
    Check(session.GetState() == FocusSession::State::Idle, "cancelled session is not idle");

    session.Start(1, 0, 0.70, zero);
    Check(session.Update(zero + 60s) == FocusSession::Event::Completed,
          "completion should take priority over skipped reminder");
    Check(session.Update(zero + 61s) == FocusSession::Event::None, "completion repeated");
}

void TestProgressPersistence()
{
    std::filesystem::path root;
    std::random_device random;
    for (int attempt = 0; attempt < 10; ++attempt) {
        root = std::filesystem::temp_directory_path() /
            ("equ-kloku-test-" + std::to_string(random()));
        if (std::filesystem::create_directory(root)) break;
        if (attempt == 9) throw std::runtime_error("Cannot create test directory");
    }

#ifdef _WIN32
    const wchar_t* oldDataHome = _wgetenv(L"APPDATA");
    const std::wstring oldValue = oldDataHome ? oldDataHome : L"";
    _wputenv_s(L"APPDATA", root.wstring().c_str());
    const auto restoreEnvironment = [&] { _wputenv_s(L"APPDATA", oldValue.c_str()); };
    const std::filesystem::path progressPath = root / "equ-kloku" / "progress.txt";
#elif defined(__APPLE__)
    const char* oldDataHome = std::getenv("HOME");
    const std::string oldValue = oldDataHome ? oldDataHome : "";
    setenv("HOME", root.c_str(), 1);
    const auto restoreEnvironment = [&] {
        if (oldDataHome) setenv("HOME", oldValue.c_str(), 1);
        else unsetenv("HOME");
    };
    const std::filesystem::path progressPath = root / "Library" /
        "Application Support" / "equ-kloku" / "progress.txt";
#else
    const char* oldDataHome = std::getenv("XDG_DATA_HOME");
    const std::string oldValue = oldDataHome ? oldDataHome : "";
    setenv("XDG_DATA_HOME", root.c_str(), 1);
    const auto restoreEnvironment = [&] {
        if (oldDataHome) setenv("XDG_DATA_HOME", oldValue.c_str(), 1);
        else unsetenv("XDG_DATA_HOME");
    };
    const std::filesystem::path progressPath = root / "equ-kloku" / "progress.txt";
#endif

    try {
        Check(LoadProgress().count == 0, "new progress should begin at zero");
        Check(SaveProgress(1).empty(), "saving progress failed");
        Check(LoadProgress().count == 1, "saved count did not reload");
        Check(SaveProgress(1).empty(), "saving same count failed");
        Check(LoadProgress().count == 1, "acknowledgement should not add count");

        std::ofstream corrupt(progressPath, std::ios::trunc);
        corrupt << "not a number\n";
        corrupt.close();
        Check(!LoadProgress().error.empty(), "corrupt progress should be reported");

        std::ofstream negative(progressPath, std::ios::trunc);
        negative << "-1\n";
        negative.close();
        Check(!LoadProgress().error.empty(), "negative progress should be rejected");
    } catch (...) {
        restoreEnvironment();
        std::filesystem::remove_all(root);
        throw;
    }

    restoreEnvironment();
    std::filesystem::remove_all(root);
}
}

int main()
{
    try {
        TestDurationAndRandomBounds();
        TestReminderPauseAndCompletion();
        TestCancelAndSimultaneousThresholds();
        TestProgressPersistence();
        std::cout << "All focus tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}

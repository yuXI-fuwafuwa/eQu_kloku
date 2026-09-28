#include "focus_session.h"
#include "progress_store.h"

#include "raylib.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace {
constexpr int kWidth = 800;
constexpr int kHeight = 450;
constexpr int kSampleRate = 44100;
constexpr double kPi = 3.14159265358979323846;

constexpr Color kBackground{249, 247, 242, 255};
constexpr Color kInk{51, 62, 66, 255};
constexpr Color kMuted{126, 132, 132, 255};
constexpr Color kGreen{100, 144, 125, 255};
constexpr Color kGreenHover{83, 128, 109, 255};
constexpr Color kPale{229, 237, 229, 255};
constexpr Color kOutline{197, 209, 199, 255};

const std::array<const char*, 18> kUiStrings{
    "已完成 次专注", "这次想专注多久？", "分钟", "开始专注", "请输入 1-999 分钟",
    "和当下待在一起", "稍作休息", "继续专注", "结束本次专注", "专注中", "已暂停",
    "这段专注完成啦", "好的", "无法读取已完成次数", "已完成次数记录格式有误",
    "完成了，但暂时无法保存次数", "音频设备不可用，提醒声音可能无法播放", "1234567890"
};

std::vector<int> UiCodepoints()
{
    std::set<int> unique;
    for (const char* label : kUiStrings) {
        while (*label) {
            int bytes = 0;
            const int codepoint = GetCodepointNext(label, &bytes);
            if (bytes <= 0) break;
            unique.insert(codepoint);
            label += bytes;
        }
    }
    return {unique.begin(), unique.end()};
}

struct UiFonts {
    Font small{};
    Font medium{};
    Font large{};
};

Font SelectFont(const UiFonts& fonts, float size)
{
    if (size <= 19.0f) return fonts.small;
    if (size <= 34.0f) return fonts.medium;
    return fonts.large;
}

UiFonts LoadUiFonts()
{
    const auto codepoints = UiCodepoints();
    constexpr std::array<const char*, 3> candidates{
        "/usr/share/fonts/adobe-source-han-sans/SourceHanSansCN-Regular.otf",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/maple/MapleMono-NF-CN-Medium.ttf"
    };
    for (const char* path : candidates) {
        if (!FileExists(path)) continue;
        UiFonts fonts;
        fonts.small = LoadFontEx(path, 19, codepoints.data(), static_cast<int>(codepoints.size()));
        fonts.medium = LoadFontEx(path, 30, codepoints.data(), static_cast<int>(codepoints.size()));
        fonts.large = LoadFontEx(path, 40, codepoints.data(), static_cast<int>(codepoints.size()));
        if (IsFontValid(fonts.small) && IsFontValid(fonts.medium) && IsFontValid(fonts.large)) {
            SetTextureFilter(fonts.small.texture, TEXTURE_FILTER_BILINEAR);
            SetTextureFilter(fonts.medium.texture, TEXTURE_FILTER_BILINEAR);
            SetTextureFilter(fonts.large.texture, TEXTURE_FILTER_BILINEAR);
            return fonts;
        }
        if (IsFontValid(fonts.small)) UnloadFont(fonts.small);
        if (IsFontValid(fonts.medium)) UnloadFont(fonts.medium);
        if (IsFontValid(fonts.large)) UnloadFont(fonts.large);
    }
    return {};
}

void DrawCentered(const UiFonts& fonts, const std::string& label, float centerX, float y,
                  float size, Color color)
{
    const Font font = SelectFont(fonts, size);
    const Vector2 extent = MeasureTextEx(font, label.c_str(), size, 1.0f);
    DrawTextEx(font, label.c_str(), {centerX - extent.x / 2.0f, y}, size, 1.0f, color);
}

bool Button(Rectangle bounds, const UiFonts& fonts, const char* label, bool pressed, Vector2 mouse,
            Color fill = kPale, Color textColor = kInk)
{
    const bool hovered = CheckCollisionPointRec(mouse, bounds);
    DrawRectangleRounded(bounds, 0.35f, 24, hovered ? kOutline : fill);
    const float fontSize = 23.0f;
    const Font font = SelectFont(fonts, fontSize);
    const Vector2 extent = MeasureTextEx(font, label, fontSize, 1.0f);
    DrawTextEx(font, label,
               {bounds.x + (bounds.width - extent.x) / 2.0f,
                bounds.y + (bounds.height - extent.y) / 2.0f},
               fontSize, 1.0f, textColor);
    return hovered && pressed;
}

bool StartButton(const UiFonts& fonts, bool pressed, Vector2 mouse)
{
    constexpr Vector2 center{400.0f, 306.0f};
    constexpr float radius = 64.0f;
    const bool hovered = CheckCollisionPointCircle(mouse, center, radius);
    DrawCircleSector(center, radius, 0.0f, 360.0f, 96, hovered ? kGreenHover : kGreen);
    DrawCentered(fonts, "开始专注", center.x, center.y - 15.0f, 25.0f, WHITE);
    return hovered && pressed;
}

std::vector<std::int16_t> MakeChime(bool completion)
{
    const double length = completion ? 1.55 : 1.0;
    const std::size_t frames = static_cast<std::size_t>(length * kSampleRate);
    std::vector<std::int16_t> samples(frames);
    const std::array<double, 3> notes = completion
        ? std::array<double, 3>{523.25, 659.25, 783.99}
        : std::array<double, 3>{523.25, 659.25, 0.0};
    for (std::size_t i = 0; i < frames; ++i) {
        const double t = static_cast<double>(i) / kSampleRate;
        double mixed = 0.0;
        for (std::size_t n = 0; n < notes.size(); ++n) {
            if (notes[n] == 0.0) continue;
            const double start = static_cast<double>(n) * (completion ? 0.30 : 0.24);
            if (t < start) continue;
            const double age = t - start;
            const double attack = std::min(1.0, age / 0.025);
            const double envelope = attack * std::exp(-3.1 * age);
            const double tone = std::sin(2.0 * kPi * notes[n] * age) +
                                0.22 * std::sin(2.0 * kPi * notes[n] * 2.0 * age);
            mixed += tone * envelope;
        }
        samples[i] = static_cast<std::int16_t>(std::clamp(mixed * 6800.0, -32000.0, 32000.0));
    }
    return samples;
}

Sound LoadChime(bool completion)
{
    std::vector<std::int16_t> samples = MakeChime(completion);
    Wave wave{static_cast<unsigned int>(samples.size()), kSampleRate, 16, 1, samples.data()};
    return LoadSoundFromWave(wave);
}

int ParseMinutes(const std::string& input)
{
    if (input.empty()) return 0;
    int value = 0;
    for (char digit : input) value = value * 10 + (digit - '0');
    return value >= 1 && value <= 999 ? value : 0;
}
}

int main()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(kWidth, kHeight, "eQu Kloku");
    if (!IsWindowReady()) {
        std::cerr << "Unable to open a graphical window. Check the display server.\n";
        return 1;
    }
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    UiFonts fonts = LoadUiFonts();
    if (!IsFontValid(fonts.small) || !IsFontValid(fonts.medium) || !IsFontValid(fonts.large)) {
        std::cerr << "No supported Chinese font found. Install Source Han Sans CN.\n";
        CloseWindow();
        return 1;
    }

    InitAudioDevice();
    const bool audioReady = IsAudioDeviceReady();
    Sound gentle{};
    Sound complete{};
    if (audioReady) {
        gentle = LoadChime(false);
        complete = LoadChime(true);
    }

    ProgressResult progress = LoadProgress();
    std::uint64_t completedCount = progress.count;
    std::string notice = progress.error;
    if (!audioReady && notice.empty()) notice = "音频设备不可用，提醒声音可能无法播放";

    std::random_device randomSeed;
    std::mt19937 random(randomSeed());
    std::uniform_int_distribution<int> minuteOffset(-10, 10);
    std::uniform_real_distribution<double> reminderRatio(0.65, 0.75);

    FocusSession session;
    enum class Screen { Home, Focus, Finished };
    Screen screen = Screen::Home;
    std::string input;
    bool inputFocused = true;
    bool invalidInput = false;

    while (!WindowShouldClose()) {
        const auto now = FocusSession::Clock::now();
        const Vector2 mouse = GetMousePosition();
        const bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        if (screen == Screen::Focus) {
            const FocusSession::Event event = session.Update(now);
            if (event == FocusSession::Event::GentleReminder && IsSoundValid(gentle)) {
                PlaySound(gentle);
            } else if (event == FocusSession::Event::Completed) {
                if (IsSoundValid(complete)) PlaySound(complete);
                if (completedCount < std::numeric_limits<std::uint64_t>::max()) ++completedCount;
                notice = SaveProgress(completedCount);
                screen = Screen::Finished;
            }
        }

        const Rectangle inputBox{285.0f, 176.0f, 230.0f, 55.0f};
        if (screen == Screen::Home) {
            if (clicked) inputFocused = CheckCollisionPointRec(mouse, inputBox);
            if (inputFocused) {
                int codepoint = GetCharPressed();
                while (codepoint > 0) {
                    if (codepoint >= '0' && codepoint <= '9' && input.size() < 3) {
                        input.push_back(static_cast<char>(codepoint));
                        invalidInput = false;
                    }
                    codepoint = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && !input.empty()) {
                    input.pop_back();
                    invalidInput = false;
                }
            }
        }

        const bool submitByKeyboard = screen == Screen::Home && IsKeyPressed(KEY_ENTER);
        BeginDrawing();
        ClearBackground(kBackground);

        if (screen == Screen::Home) {
            DrawCentered(fonts, "已完成 " + std::to_string(completedCount) + " 次专注",
                         400.0f, 51.0f, 26.0f, kMuted);
            DrawCentered(fonts, "这次想专注多久？", 400.0f, 120.0f, 30.0f, kInk);

            DrawRectangleRounded(inputBox, 0.26f, 24, WHITE);
            DrawRectangleRoundedLinesEx(inputBox, 0.26f, 24, 2.0f,
                                        inputFocused ? kGreen : kOutline);
            DrawCentered(fonts, input, 400.0f, 183.0f, 33.0f, kInk);
            DrawTextEx(SelectFont(fonts, 20.0f), "分钟", {531.0f, 189.0f}, 20.0f, 1.0f, kMuted);
            if (inputFocused && input.empty() && static_cast<int>(GetTime() * 2.0) % 2 == 0) {
                DrawRectangle(398, 188, 2, 31, kGreen);
            }

            const bool submitByMouse = StartButton(fonts, clicked, mouse);
            if (invalidInput) DrawCentered(fonts, "请输入 1-999 分钟", 400.0f, 384.0f, 17.0f, kMuted);
            else if (!notice.empty()) DrawCentered(fonts, notice, 400.0f, 389.0f, 15.0f, kMuted);

            if (submitByMouse || submitByKeyboard) {
                const int minutes = ParseMinutes(input);
                if (minutes == 0) {
                    invalidInput = true;
                } else {
                    session.Start(minutes, minuteOffset(random), reminderRatio(random), now);
                    screen = Screen::Focus;
                    inputFocused = false;
                    invalidInput = false;
                    notice.clear();
                }
            }
        } else if (screen == Screen::Focus) {
            DrawCentered(fonts, "和当下待在一起", 400.0f, 91.0f, 38.0f, kInk);
            const char* pauseLabel = session.GetState() == FocusSession::State::Paused
                ? "继续专注" : "稍作休息";
            const bool togglePause = Button({265.0f, 213.0f, 270.0f, 57.0f}, fonts,
                                            pauseLabel, clicked, mouse);
            const bool abandon = Button({265.0f, 285.0f, 270.0f, 57.0f}, fonts,
                                        "结束本次专注", clicked, mouse, {241, 239, 235, 255});
            DrawCentered(fonts, session.GetState() == FocusSession::State::Paused
                         ? "已暂停" : "专注中", 400.0f, 404.0f, 17.0f, kMuted);

            if (togglePause) {
                if (session.GetState() == FocusSession::State::Paused) session.Resume(now);
                else session.Pause(now);
            } else if (abandon) {
                session.Cancel();
                screen = Screen::Home;
                inputFocused = true;
            }
        } else {
            DrawCentered(fonts, "这段专注完成啦", 400.0f, 131.0f, 40.0f, kInk);
            const bool acknowledged = Button({321.0f, 252.0f, 158.0f, 62.0f}, fonts,
                                             "好的", clicked, mouse);
            if (!notice.empty()) DrawCentered(fonts, notice, 400.0f, 367.0f, 16.0f, kMuted);
            if (acknowledged) {
                session.Cancel();
                screen = Screen::Home;
                inputFocused = true;
            }
        }

        EndDrawing();
        SetMouseCursor(screen == Screen::Home && CheckCollisionPointRec(mouse, inputBox)
                           ? MOUSE_CURSOR_IBEAM : MOUSE_CURSOR_DEFAULT);
    }

    if (IsSoundValid(gentle)) UnloadSound(gentle);
    if (IsSoundValid(complete)) UnloadSound(complete);
    if (audioReady) CloseAudioDevice();
    UnloadFont(fonts.small);
    UnloadFont(fonts.medium);
    UnloadFont(fonts.large);
    CloseWindow();
    return 0;
}

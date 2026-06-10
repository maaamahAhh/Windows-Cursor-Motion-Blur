#pragma once

#include <windows.h>
#include <vector>
#include <chrono>

constexpr float DEFAULT_TRAIL_DURATION_MS = 80.0f;
constexpr int MAX_HISTORY_SIZE = 256;

struct MotionBlurConfig {
    int ghostCount;
    float baseOpacity;
    float trailDurationMs;

    MotionBlurConfig()
        : ghostCount(30)
        , baseOpacity(0.15f)
        , trailDurationMs(DEFAULT_TRAIL_DURATION_MS)
    {}
};

class MotionBlur {
public:
    MotionBlur();

    void initialize();
    void addCursorPosition(POINT cursorPosition);
    std::vector<POINT> getGhostPositions() const;
    float ghostOpacityFor(int ghostIndex) const;
    MotionBlurConfig& getConfig();
    const MotionBlurConfig& getConfig() const;
    void clear();

private:
    struct PositionRecord {
        POINT position;
        std::chrono::steady_clock::time_point timestamp;
    };

    POINT interpolatedPositionAt(
        std::chrono::steady_clock::time_point targetTime) const;
    void trimHistory();

    std::vector<PositionRecord> positionHistory_;
    MotionBlurConfig config_;
};

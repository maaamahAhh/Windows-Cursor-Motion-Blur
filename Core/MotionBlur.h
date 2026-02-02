#pragma once

#include <windows.h>
#include <vector>
#include <chrono>

struct PositionRecord {
    POINT position;
    std::chrono::steady_clock::time_point timestamp;
};

struct MotionBlurConfig {
    int numCopies;
    float baseOpacity;
    float delayPerCopy;
    float duration;
    bool hideOriginalCursor;
    bool enableSpeedAdaptive;
    float minDelayPerCopy;
    float maxDelayPerCopy;
    int minCopies;
    int maxCopies;

    MotionBlurConfig()
        : numCopies(60)
        , baseOpacity(0.06f)
        , delayPerCopy(0.0007f)
        , duration(0.05f)
        , hideOriginalCursor(false)
        , enableSpeedAdaptive(true)
        , minDelayPerCopy(0.0003f)
        , maxDelayPerCopy(0.001f)
        , minCopies(40)
        , maxCopies(80)
    {}
};

class MotionBlur {
public:
    MotionBlur();
    ~MotionBlur();

    void Initialize();
    void AddPosition(POINT pos);
    std::vector<POINT> GetCursorCopyPositions();

    MotionBlurConfig& GetConfig();
    const MotionBlurConfig& GetConfig() const;

    HBITMAP GetCursorBitmap() const;
    void SetCursorBitmap(HBITMAP hbm);

    void Clear();

private:
    float CalculateSpeed();
    void AdjustParametersBySpeed(float speed);

    std::vector<PositionRecord> positionHistory_;
    MotionBlurConfig config_;
    HBITMAP hCursorBitmap_;
    float currentSpeed_;
    POINT lastPosition_;
    std::chrono::steady_clock::time_point lastTime_;
};
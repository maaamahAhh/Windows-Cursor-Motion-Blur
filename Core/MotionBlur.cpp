#include "MotionBlur.h"
#include <algorithm>
#include <cmath>

MotionBlur::MotionBlur()
    : hCursorBitmap_(nullptr)
    , currentSpeed_(0.0f)
    , lastPosition_{0, 0}
{
    lastTime_ = std::chrono::steady_clock::now();
}

MotionBlur::~MotionBlur() {
    Clear();
}

void MotionBlur::Initialize() {
    positionHistory_.clear();
    currentSpeed_ = 0.0f;
    lastPosition_ = {0, 0};
    lastTime_ = std::chrono::steady_clock::now();

    if (!hCursorBitmap_) {
        HDC hdc = GetDC(nullptr);
        hCursorBitmap_ = CreateCompatibleBitmap(hdc, 32, 32);
        ReleaseDC(nullptr, hdc);
    }
}

void MotionBlur::AddPosition(POINT pos) {
    auto now = std::chrono::steady_clock::now();

    if (config_.enableSpeedAdaptive) {
        currentSpeed_ = CalculateSpeed();
        AdjustParametersBySpeed(currentSpeed_);
    }

    PositionRecord record;
    record.position = pos;
    record.timestamp = now;
    positionHistory_.push_back(record);

    float maxDelay = config_.numCopies * config_.maxDelayPerCopy;
    size_t maxHistorySize = static_cast<size_t>(maxDelay * 1000) + 100;

    if (positionHistory_.size() > maxHistorySize) {
        positionHistory_.erase(positionHistory_.begin());
    }

    lastPosition_ = pos;
    lastTime_ = now;
}

float MotionBlur::CalculateSpeed() {
    if (positionHistory_.size() < 2) {
        return 0.0f;
    }

    size_t sampleCount = std::min(size_t(5), positionHistory_.size());
    auto& last = positionHistory_.back();
    auto& first = positionHistory_[positionHistory_.size() - sampleCount];

    auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(
        last.timestamp - first.timestamp
    ).count();

    if (timeDiff <= 0) {
        return 0.0f;
    }

    int dx = last.position.x - first.position.x;
    int dy = last.position.y - first.position.y;
    float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));

    return distance / static_cast<float>(timeDiff);
}

void MotionBlur::AdjustParametersBySpeed(float speed) {
    float normalizedSpeed = std::min(speed / 5.0f, 1.0f);

    config_.delayPerCopy = config_.maxDelayPerCopy - normalizedSpeed * (config_.maxDelayPerCopy - config_.minDelayPerCopy);

    config_.numCopies = static_cast<int>(config_.minCopies + normalizedSpeed * (config_.maxCopies - config_.minCopies));
}

std::vector<POINT> MotionBlur::GetCursorCopyPositions() {
    std::vector<POINT> positions;
    positions.resize(config_.numCopies);

    if (positionHistory_.empty()) {
        return positions;
    }

    auto now = std::chrono::steady_clock::now();

    for (int i = 0; i < config_.numCopies; ++i) {
        float delay = static_cast<float>(i) * config_.delayPerCopy;
        auto targetTime = now - std::chrono::milliseconds(static_cast<long long>(delay * 1000));

        auto it = std::lower_bound(
            positionHistory_.begin(),
            positionHistory_.end(),
            targetTime,
            [](const PositionRecord& record, const auto& time) {
                return record.timestamp < time;
            }
        );

        if (it != positionHistory_.end()) {
            positions[i] = it->position;
        } else {
            positions[i] = positionHistory_.back().position;
        }
    }

    return positions;
}

MotionBlurConfig& MotionBlur::GetConfig() {
    return config_;
}

const MotionBlurConfig& MotionBlur::GetConfig() const {
    return config_;
}

HBITMAP MotionBlur::GetCursorBitmap() const {
    return hCursorBitmap_;
}

void MotionBlur::SetCursorBitmap(HBITMAP hbm) {
    if (hCursorBitmap_) {
        DeleteObject(hCursorBitmap_);
    }
    hCursorBitmap_ = hbm;
}

void MotionBlur::Clear() {
    if (hCursorBitmap_) {
        DeleteObject(hCursorBitmap_);
        hCursorBitmap_ = nullptr;
    }
    positionHistory_.clear();
}
#include "MotionBlur.h"

MotionBlur::MotionBlur() = default;

void MotionBlur::initialize() {
    clear();
}

void MotionBlur::addCursorPosition(POINT cursorPosition) {
    PositionRecord record;
    record.position = cursorPosition;
    record.timestamp = std::chrono::steady_clock::now();
    positionHistory_.push_back(record);
    trimHistory();
}

std::vector<POINT> MotionBlur::getGhostPositions() const {
    std::vector<POINT> positions(config_.ghostCount);
    if (positionHistory_.empty() || config_.ghostCount <= 1) {
        if (!positionHistory_.empty()) {
            for (auto& pos : positions) {
                pos = positionHistory_.back().position;
            }
        }
        return positions;
    }

    auto now = std::chrono::steady_clock::now();

    for (int i = 0; i < config_.ghostCount; ++i) {
        float timeOffsetMs = (static_cast<float>(i) / (config_.ghostCount - 1))
                             * config_.trailDurationMs;
        auto targetTime = now - std::chrono::microseconds(
            static_cast<long long>(timeOffsetMs * 1000.0f));
        positions[i] = interpolatedPositionAt(targetTime);
    }

    return positions;
}

POINT MotionBlur::interpolatedPositionAt(
    std::chrono::steady_clock::time_point targetTime) const {

    if (positionHistory_.size() == 1) {
        return positionHistory_.front().position;
    }
    if (targetTime >= positionHistory_.back().timestamp) {
        return positionHistory_.back().position;
    }
    if (targetTime <= positionHistory_.front().timestamp) {
        return positionHistory_.front().position;
    }

    // Binary search: find first entry with timestamp >= targetTime
    size_t lo = 0, hi = positionHistory_.size() - 1;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (positionHistory_[mid].timestamp < targetTime) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    if (lo == 0) {
        return positionHistory_.front().position;
    }

    const auto& before = positionHistory_[lo - 1];
    const auto& after = positionHistory_[lo];

    auto span = std::chrono::duration_cast<std::chrono::microseconds>(
        after.timestamp - before.timestamp).count();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        targetTime - before.timestamp).count();

    float fraction = (span > 0)
        ? static_cast<float>(elapsed) / static_cast<float>(span)
        : 0.0f;

    POINT result;
    result.x = before.position.x + static_cast<int>(
        (after.position.x - before.position.x) * fraction);
    result.y = before.position.y + static_cast<int>(
        (after.position.y - before.position.y) * fraction);
    return result;
}

void MotionBlur::trimHistory() {
    while (positionHistory_.size() > static_cast<size_t>(MAX_HISTORY_SIZE)) {
        positionHistory_.erase(positionHistory_.begin());
    }
}

MotionBlurConfig& MotionBlur::getConfig() {
    return config_;
}

const MotionBlurConfig& MotionBlur::getConfig() const {
    return config_;
}

void MotionBlur::clear() {
    positionHistory_.clear();
}

float MotionBlur::ghostOpacityFor(int ghostIndex) const {
    if (config_.ghostCount <= 1) {
        return config_.baseOpacity;
    }

    float normalizedPosition = static_cast<float>(ghostIndex)
        / static_cast<float>(config_.ghostCount - 1);

    // Oldest ghost is 20% of baseOpacity, newest ghost is 100% of baseOpacity
    return config_.baseOpacity * (0.2f + 0.8f * normalizedPosition);
}

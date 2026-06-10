#include "MotionBlur.h"

MotionBlur::MotionBlur() = default;

void MotionBlur::initialize() {
    positionHistory_.clear();
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
    if (positionHistory_.empty()) return positions;

    auto now = std::chrono::steady_clock::now();

    for (int i = 0; i < config_.ghostCount; ++i) {
        float timeOffsetMs = (positionHistory_.size() > 1)
            ? (static_cast<float>(i) / (config_.ghostCount - 1))
              * config_.trailDurationMs
            : 0.0f;

        auto targetTime = now - std::chrono::microseconds(
            static_cast<long long>(timeOffsetMs * 1000.0f));
        positions[i] = interpolatedPositionAt(targetTime);
    }

    return positions;
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

    for (size_t i = 0; i + 1 < positionHistory_.size(); ++i) {
        if (positionHistory_[i + 1].timestamp >= targetTime) {
            auto& before = positionHistory_[i];
            auto& after = positionHistory_[i + 1];

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
    }

    return positionHistory_.back().position;
}

void MotionBlur::trimHistory() {
    if (positionHistory_.size() > static_cast<size_t>(MAX_HISTORY_SIZE)) {
        positionHistory_.erase(positionHistory_.begin());
    }
}

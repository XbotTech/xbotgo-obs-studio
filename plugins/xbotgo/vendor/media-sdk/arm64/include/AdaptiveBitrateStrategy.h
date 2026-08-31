#pragma once

#include "PushStrategy.h"
#include "NodeConfigs.h"
#include "NodeMessages.h"
#include <vector>
#include <deque>
#include <cstdint>

namespace blink {
namespace media {

// Represents a video encoding quality level
struct BitrateLevel {
    int32_t width;
    int32_t height;
    int32_t fps;
    int32_t bitrate;  // Target bitrate for this level

    BitrateLevel() : width(0), height(0), fps(0), bitrate(0) {}
    BitrateLevel(int32_t w, int32_t h, int32_t f, int32_t b)
        : width(w), height(h), fps(f), bitrate(b) {}
};

// Adaptive bitrate strategy that adjusts video encoding based on network conditions
//
// Strategy logic:
// 1. Congestion (downgrade): RTT > 100ms OR bandwidth < 90% of target OR RTT rises >5ms for 2 consecutive intervals
// 2. Recovery (hold): current RTT drops >5ms — hold current bitrate, no upgrade
// 3. Stable (probe): current RTT change within ±5ms — increase bitrate or upgrade level
//
class AdaptiveBitrateStrategy : public IPushStrategy {
public:
    // Constructor takes a vector of bitrate levels, ordered from highest to lowest quality
    // levels[0] should be the highest quality, levels[n-1] should be the lowest quality
    AdaptiveBitrateStrategy(const std::vector<BitrateLevel> &levels);
    virtual ~AdaptiveBitrateStrategy();

    // IPushStrategy interface
    virtual int onPushStreamStart(IPushStream *pusher, PushStreamConfig &cfg) override;
    virtual int onPushStreamStop() override;
    virtual int onSrtPushStates(const SrtPushStatesMessage &msg) override;

private:
    // Find appropriate level index based on actual bitrate
    int findLevelByBitrate(int64_t actualBitrate);

    // Apply video encoder config for the given level
    void applyLevel(int levelIndex);

    void adjustVideoBitrate(int64_t bitrate);

    double calculateExtraBitratePercent(int32_t fecCols, int32_t fecRows);

    // Returns average of the sliding window for the given level, or 1.0 if no data yet
    // double getBitrateCoeff(int levelIndex) const;

    IPushStream *m_pusher;
    std::vector<BitrateLevel> m_levels;  // Quality levels, ordered high to low
    int m_currentLevel;                   // Current level index (0 = highest quality)
    int64_t m_videoTargetBitrate;         // Target bitrate for the current level (video only)
    int64_t m_pacedSenderBitrate;       // Current target bitrate (may differ from level bitrate), including audio bitrate
    int64_t m_lastStateTimeMs;            // Timestamp of last state update

    SrtPacketLossResilienceLevel m_srtPacketLossResilienceLevel;
    double m_extraBitratePercent; // Extra bitrate percentage for FEC overhead, calculated based on FEC config
    int32_t m_fecPercentNum;
    int32_t m_fecPercentDen;

    int64_t m_audioBitrate;

    double m_lastRttMs;               // RTT value from previous state update
    int32_t m_consecutiveRttIncreases; // Number of consecutive intervals where RTT increased

    std::vector<std::deque<double>> m_bitrateCoeffWindows; // Per-level sliding window of recent bitrate coefficients (last 3 measurements)
    uint32_t m_videoSsrc;                                  // Video SSRC, used to extract video-only bytes from SrtPushStatesMessage
    uint32_t m_audioSsrc;                                  // Audio SSRC, used to check audio presence in SrtPushStatesMessage
};

} // namespace media
} // namespace blink

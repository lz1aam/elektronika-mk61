#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace pmk {

// Converts host elapsed time to MK-61 microticks without reading a clock or
// depending on a UI framework. The host supplies monotonic elapsed durations.
class RealtimeClock {
public:
    static constexpr std::uint64_t TicksPerSecond = 168'000;
    static constexpr std::uint64_t MaximumElapsedNanoseconds = 100'000'000;

    void reset() { remainder_ = 0; }

    // Long pauses discard excess elapsed time to bound resume work. Within
    // this limit fractional ticks carry across calls, independent of cadence.
    std::size_t consumeNanoseconds(std::uint64_t elapsed) {
        constexpr std::uint64_t NanosecondsPerSecond = 1'000'000'000;
        elapsed = std::min(elapsed, MaximumElapsedNanoseconds);
        const auto accumulated = remainder_ + elapsed * TicksPerSecond;
        remainder_ = accumulated % NanosecondsPerSecond;
        return static_cast<std::size_t>(accumulated / NanosecondsPerSecond);
    }

private:
    std::uint64_t remainder_{};
};

} // namespace pmk

#ifndef VSOMEIP_V3_INSTRUMENTATION_HPP_
#define VSOMEIP_V3_INSTRUMENTATION_HPP_

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <thread>
#include <vsomeip/internal/logger.hpp>

namespace vsomeip_v3 {
namespace bench_internal {

inline bool enabled() {
    static int st = -1;
    if (st == -1) {
        const char* e = std::getenv("VSOMEIP_BENCH_INTERNAL");
        st = (e && std::string(e) == "1") ? 1 : 0;
    }
    return st == 1;
}

inline std::uint64_t now_ns() {
    using clock_type = std::chrono::steady_clock;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(clock_type::now().time_since_epoch()).count();
}

struct scope_timer {
    const char* name_;
    std::uint64_t start_;
    std::size_t bytes_;
    scope_timer(const char* name, std::size_t bytes = 0)
        : name_(name), start_(enabled() ? now_ns() : 0), bytes_(bytes) {}
    ~scope_timer() {
        if (enabled()) {
            auto elapsed = now_ns() - start_;
            VSOMEIP_INFO << "bench-internal name=" << name_ << " ns=" << elapsed << " bytes=" << bytes_;
        }
    }
};

inline void log_event(const char* name, std::uint64_t ns, std::size_t bytes) {
    if (enabled()) {
        VSOMEIP_INFO << "bench-internal name=" << name << " ns=" << ns << " bytes=" << bytes;
    }
}

}
}

#endif


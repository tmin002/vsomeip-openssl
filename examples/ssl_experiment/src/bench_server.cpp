#include <vsomeip/vsomeip.hpp>
#include <chrono>
#include <ctime>
#include <iostream>
#include <cstring>
#include <vector>

using clock_type = std::chrono::steady_clock;

static std::shared_ptr<vsomeip::application> app;
static constexpr vsomeip::service_t SERVICE = 0x9001;
static constexpr vsomeip::instance_t INSTANCE = 0x0001;
static constexpr vsomeip::method_t METHOD = 0x0001;

struct timestamps_t {
    uint64_t t_recv_in;
    uint64_t t_before_resp;
    uint64_t cpu_ns; // server CPU time consumed between recv and before_resp
};

static inline uint64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(clock_type::now().time_since_epoch()).count();
}

static inline uint64_t proc_cpu_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<uint64_t>(ts.tv_nsec);
}

void handle_request(const std::shared_ptr<vsomeip::message> &request) {
    timestamps_t ts{};
    ts.t_recv_in = now_ns();
    uint64_t cpu_start = proc_cpu_ns();

    // Echo payload back and attach server timing in the beginning
    auto req_pl = request->get_payload();
    const auto len = req_pl->get_length();
    const auto data_ptr = req_pl->get_data();

    std::vector<vsomeip::byte_t> out;
    out.resize(sizeof(timestamps_t) + len);
    std::memcpy(out.data(), &ts, sizeof(ts));
    if (len) std::memcpy(out.data() + sizeof(timestamps_t), data_ptr, len);

    auto resp = vsomeip::runtime::get()->create_response(request);
    ts.t_before_resp = now_ns();
    ts.cpu_ns = proc_cpu_ns() - cpu_start;
    std::memcpy(out.data(), &ts, sizeof(ts));
    auto pl = vsomeip::runtime::get()->create_payload();
    pl->set_data(out);
    resp->set_payload(pl);
    app->send(resp);
}

void on_state(vsomeip::state_type_e state) {
    if (state == vsomeip::state_type_e::ST_REGISTERED) {
        app->offer_service(SERVICE, INSTANCE);
    }
}

int main() {
    app = vsomeip::runtime::get()->create_application("bench_server");
    if (!app->init()) return 1;
    app->register_state_handler(on_state);
    app->register_message_handler(SERVICE, INSTANCE, METHOD, handle_request);
    app->start();
    return 0;
}


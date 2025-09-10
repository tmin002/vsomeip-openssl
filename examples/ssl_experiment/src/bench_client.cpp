#include <vsomeip/vsomeip.hpp>
#include <chrono>
#include <cstring>
#include <iostream>
#include <numeric>
#include <vector>

using clock_type = std::chrono::steady_clock;

static std::shared_ptr<vsomeip::application> app;
static constexpr vsomeip::service_t SERVICE = 0x9001;
static constexpr vsomeip::instance_t INSTANCE = 0x0001;
static constexpr vsomeip::method_t METHOD = 0x0001;

struct timestamps_t {
    uint64_t t_recv_in;      // server received
    uint64_t t_before_resp;  // server right before send
    uint64_t cpu_ns;         // server CPU time spent
};

static inline uint64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(clock_type::now().time_since_epoch()).count();
}

static std::size_t current_size = 1024; // bytes
static std::size_t max_size = 1024 * 1024; // 1 MiB
static int messages_per_size = 100;

static uint64_t t_client_send = 0;

void send_one() {
    auto req = vsomeip::runtime::get()->create_request();
    req->set_service(SERVICE);
    req->set_instance(INSTANCE);
    req->set_method(METHOD);
    std::vector<vsomeip::byte_t> payload(current_size, 0xAB);
    auto pl = vsomeip::runtime::get()->create_payload();
    pl->set_data(payload);
    req->set_payload(pl);
    t_client_send = now_ns();
    app->send(req);
}

static int sent_count = 0;
static std::vector<uint64_t> rtts_ns;
static std::vector<uint64_t> server_proc_ns;
static std::vector<uint64_t> server_que_ns;

void on_response(const std::shared_ptr<vsomeip::message> &resp) {
    auto t_recv = now_ns();
    auto pl = resp->get_payload();
    if (pl->get_length() < sizeof(timestamps_t)) return;
    timestamps_t ts{};
    std::memcpy(&ts, pl->get_data(), sizeof(ts));
    uint64_t rtt = t_recv - t_client_send;
    uint64_t s_proc = ts.t_before_resp - ts.t_recv_in;
    uint64_t s_queue = ts.t_recv_in - t_client_send; // includes network + vsomeip queueing
    rtts_ns.push_back(rtt);
    server_proc_ns.push_back(s_proc);
    server_que_ns.push_back(s_queue);

    if (++sent_count < messages_per_size) {
        send_one();
        return;
    }

    auto avg = [](const std::vector<uint64_t>& v){
        return v.empty() ? 0ULL : std::accumulate(v.begin(), v.end(), 0ULL) / static_cast<uint64_t>(v.size());
    };
    std::cout << "size=" << current_size/1024 << "KB "
              << "rtt_avg(ns)=" << avg(rtts_ns) << " "
              << "srv_proc_avg(ns)=" << avg(server_proc_ns) << " "
              << "srv_queue_est(ns)=" << avg(server_que_ns) << std::endl;

    rtts_ns.clear(); server_proc_ns.clear(); server_que_ns.clear();
    sent_count = 0;
    current_size *= 2;
    if (current_size <= max_size) {
        send_one();
    } else {
        std::cout << "benchmark done" << std::endl;
        app->stop();
    }
}

void on_state(vsomeip::state_type_e state) {
    if (state == vsomeip::state_type_e::ST_REGISTERED) {
        app->request_service(SERVICE, INSTANCE);
    }
}

void on_availability(vsomeip::service_t s, vsomeip::instance_t i, bool av) {
    if (s==SERVICE && i==INSTANCE && av) {
        current_size = 1024; sent_count = 0; rtts_ns.clear(); server_proc_ns.clear(); server_que_ns.clear();
        send_one();
    }
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    app = vsomeip::runtime::get()->create_application("bench_client");
    if (!app->init()) return 1;
    app->register_state_handler(on_state);
    app->register_availability_handler(SERVICE, INSTANCE, on_availability);
    app->register_message_handler(SERVICE, INSTANCE, METHOD, on_response);
    app->start();
    return 0;
}


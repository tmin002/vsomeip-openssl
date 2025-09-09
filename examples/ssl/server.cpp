#include <vsomeip/vsomeip.hpp>
#include <csignal>
#include <cstdlib>
#include <iostream>

static std::shared_ptr<vsomeip::application> app;

static constexpr vsomeip::service_t SERVICE = 0x1234;
static constexpr vsomeip::instance_t INSTANCE = 0x5678;
static constexpr vsomeip::method_t METHOD = 0x0421;

void handle_request(const std::shared_ptr<vsomeip::message> &request) {
    auto resp = vsomeip::runtime::get()->create_response(request);
    auto pl = vsomeip::runtime::get()->create_payload();
    std::string msg = "hello from tls server";
    std::vector<vsomeip::byte_t> data(msg.begin(), msg.end());
    pl->set_data(data);
    resp->set_payload(pl);
    app->send(resp);
}

void on_state(vsomeip::state_type_e state) {
    if (state == vsomeip::state_type_e::ST_REGISTERED) {
        app->offer_service(SERVICE, INSTANCE);
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    app = vsomeip::runtime::get()->create_application("ssl_server");
    if (!app->init()) {
        std::cerr << "init failed" << std::endl;
        return 1;
    }
    app->register_state_handler(on_state);
    app->register_message_handler(SERVICE, INSTANCE, METHOD, handle_request);
    app->start();
    return 0;
}


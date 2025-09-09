#include <vsomeip/vsomeip.hpp>
#include <iostream>
#include <chrono>

static std::shared_ptr<vsomeip::application> app;

static constexpr vsomeip::service_t SERVICE = 0x1234;
static constexpr vsomeip::instance_t INSTANCE = 0x5678;
static constexpr vsomeip::method_t METHOD = 0x0421;

void on_state(vsomeip::state_type_e state) {
    if (state == vsomeip::state_type_e::ST_REGISTERED) {
        app->request_service(SERVICE, INSTANCE);
    }
}

void on_availability(vsomeip::service_t service, vsomeip::instance_t instance, bool available) {
    if (service == SERVICE && instance == INSTANCE && available) {
        auto req = vsomeip::runtime::get()->create_request();
        req->set_service(SERVICE);
        req->set_instance(INSTANCE);
        req->set_method(METHOD);
        auto pl = vsomeip::runtime::get()->create_payload();
        std::string msg = "hello from tls client";
        std::vector<vsomeip::byte_t> data(msg.begin(), msg.end());
        pl->set_data(data);
        req->set_payload(pl);
        app->send(req);
    }
}

void on_response(const std::shared_ptr<vsomeip::message> &resp) {
    auto pl = resp->get_payload();
    auto data_ptr = pl->get_data();
    auto len = pl->get_length();
    std::string s(reinterpret_cast<const char*>(data_ptr), reinterpret_cast<const char*>(data_ptr) + len);
    std::cout << "response: " << s << std::endl;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    app = vsomeip::runtime::get()->create_application("ssl_client");
    if (!app->init()) return 1;
    app->register_state_handler(on_state);
    app->register_availability_handler(SERVICE, INSTANCE, on_availability);
    app->register_message_handler(SERVICE, INSTANCE, METHOD, on_response);
    app->start();
    return 0;
}


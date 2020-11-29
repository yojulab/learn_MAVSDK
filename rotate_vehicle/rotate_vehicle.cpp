//
// Simple example to demonstrate how to use the MAVSDK.
//
// Author: Julian Oes <julian@oes.ch>

#include <iostream>
#include <thread>
#include <chrono>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <mavsdk/plugins/offboard/offboard.h>

using namespace mavsdk;
// using namespace std::this_thread;
// using namespace std::chrono;

// Handles Offboard's result
inline void offboard_error_exit(Offboard::Result result, const std::string& message)
{
    if (result != Offboard::Result::Success) {
        std::cerr << message << result << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Logs during Offboard control
inline void offboard_log(const std::string& offb_mode, const std::string msg)
{
    std::cout << "[" << offb_mode << "] " << msg << std::endl;
}

bool offb_ctrl_body(std::shared_ptr<mavsdk::Offboard> offboard)
{
    const std::string offb_mode = "BODY";

    // Send it once before starting offboard, otherwise it will be rejected.
    Offboard::VelocityBodyYawspeed stay{};
    offboard->set_velocity_body(stay);

    Offboard::Result offboard_result = offboard->start();
    offboard_error_exit(offboard_result, "Offboard start failed: ");
    offboard_log(offb_mode, "Offboard started");

    offboard_log(offb_mode, "Turn clock-wise and climb");
    Offboard::VelocityBodyYawspeed cc_and_climb{};
    cc_and_climb.down_m_s = -1.0f;
    cc_and_climb.yawspeed_deg_s = 60.0f;
    offboard->set_velocity_body(cc_and_climb);
    std::this_thread::sleep_for(std::chrono::seconds(5));

    offboard_log(offb_mode, "Turn back anti-clockwise");
    cc_and_climb.down_m_s = -1.0f;
    cc_and_climb.yawspeed_deg_s = -60.0f;
    offboard->set_velocity_body(cc_and_climb);
    std::this_thread::sleep_for(std::chrono::seconds(5));

    offboard_log(offb_mode, "Turn back anti-clockwise");
    Offboard::VelocityBodyYawspeed ccw{};
    ccw.down_m_s = -1.0f;
    ccw.yawspeed_deg_s = -60.0f;
    offboard->set_velocity_body(ccw);
    std::this_thread::sleep_for(std::chrono::seconds(5));

    offboard_log(offb_mode, "Wait for a bit");
    offboard->set_velocity_body(stay);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    offboard_log(offb_mode, "Fly a circle");
    Offboard::VelocityBodyYawspeed circle{};
    circle.forward_m_s = 5.0f;
    circle.yawspeed_deg_s = 30.0f;
    offboard->set_velocity_body(circle);
    std::this_thread::sleep_for(std::chrono::seconds(15));

    offboard_log(offb_mode, "Wait for a bit");
    offboard->set_velocity_body(stay);
    std::this_thread::sleep_for(std::chrono::seconds(5));

    offboard_log(offb_mode, "Fly a circle sideways");
    circle.right_m_s = -5.0f;
    circle.yawspeed_deg_s = 30.0f;
    offboard->set_velocity_body(circle);
    std::this_thread::sleep_for(std::chrono::seconds(15));

    offboard_log(offb_mode, "Wait for a bit");
    offboard->set_velocity_body(stay);
    std::this_thread::sleep_for(std::chrono::seconds(8));

    offboard_result = offboard->stop();
    offboard_error_exit(offboard_result, "Offboard stop failed: ");
    offboard_log(offb_mode, "Offboard stopped");

    return true;
}

int main(int argc, char** argv)
{
    Mavsdk mavsdk;
    ConnectionResult connection_result;

    bool discovered_system = false;
    if (argc == 2) {
        connection_result = mavsdk.add_any_connection(argv[1]);
    } else {
        std::cout << "Need to Connection URL format" << std::endl;
        return 1;
    }

    if (connection_result != ConnectionResult::Success) {
        std::cout << "Connection failed: " << std::endl;
        return 1;
    }

    std::cout << "Waiting to discover system..." << std::endl;
    mavsdk.subscribe_on_new_system([&mavsdk, &discovered_system]() {
        const auto system = mavsdk.systems().at(0);

        if (system->is_connected()) {
            std::cout << "Discovered system" << std::endl;
            discovered_system = true;
        }
    });

    // We usually receive heartbeats at 1Hz, therefore we should find a system after around 2
    // seconds.
    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (!discovered_system) {
        std::cout << "No system found, exiting." << std::endl;
        return 1;
    }

    const auto system = mavsdk.systems().at(0);

    // Register a callback so we get told when components (camera, gimbal) etc
    // are found.
    system->register_component_discovered_callback([](ComponentType component_type) -> void {
        std::cout << "Discovered a component with type " << unsigned(component_type) << std::endl;
    });

    auto telemetry = std::make_shared<Telemetry>(system);

    // We want to listen to the altitude of the drone at 1 Hz.
    const Telemetry::Result set_rate_result = telemetry->set_rate_position(1.0);
    if (set_rate_result != Telemetry::Result::Success) {
        std::cout << "Setting rate failed:" << set_rate_result << std::endl;
        return 1;
    }

    // Set up callback to monitor altitude while the vehicle is in flight
    telemetry->subscribe_position([](Telemetry::Position position) {
        std::cout << "Altitude: " << position.relative_altitude_m << " m" << std::endl;
    });

    // Check if vehicle is ready to arm
    while (telemetry->health_all_ok() != true) {
        std::cout << "Vehicle is getting ready to arm" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Arm vehicle
    auto action = std::make_shared<Action>(system);
    std::cout << "Arming..." << std::endl;
    const Action::Result arm_result = action->arm();

    if (arm_result != Action::Result::Success) {
        std::cout << "Arming failed:" << arm_result << std::endl;
        return 1;
    }

    // Take off
    std::cout << "Taking off..." << std::endl;
    const Action::Result takeoff_result = action->takeoff();
    if (takeoff_result != Action::Result::Success) {
        std::cout << "Takeoff failed:" << takeoff_result << std::endl;
        return 1;
    }

    auto offboard = std::make_shared<Offboard>(system);
    //  using local NED co-ordinates
    bool ret = false;
    ret = offb_ctrl_body(offboard);
    if (ret == false) {
        return EXIT_FAILURE;
    }

    // Let it hover for a bit before landing again.
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "Landing..." << std::endl;
    const Action::Result land_result = action->land();
    if (land_result != Action::Result::Success) {
        std::cout << "Land failed:" << land_result << std::endl;
        return 1;
    }

    // Check if vehicle is still in air
    while (telemetry->in_air()) {
        std::cout << "Vehicle is landing..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Landed!" << std::endl;

    // We are relying on auto-disarming but let's keep watching the telemetry for a bit longer.
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "Finished..." << std::endl;

    return 0;
}

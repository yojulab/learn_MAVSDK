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
#include <mavsdk/plugins/mission/mission.h>
#include <future>

using namespace mavsdk;

// Handles Mission's result
// void handle_mission_err_exit(Mission::Result result, const std::string& message)
// {
//     if (result != Mission::Result::Success) {
//         std::cerr << message << result << std::endl;
//         exit(EXIT_FAILURE);
//     }
// };

// static Mission::MissionItem make_mission_item(
//     double latitude_deg,
//     double longitude_deg,
//     float relative_altitude_m,
//     float speed_m_s,
//     bool is_fly_through,
//     float gimbal_pitch_deg,
//     float gimbal_yaw_deg,
//     Mission::MissionItem::CameraAction camera_action);

// Mission::MissionItem make_mission_item(
//     double latitude_deg,
//     double longitude_deg,
//     float relative_altitude_m,
//     float speed_m_s,
//     bool is_fly_through,
//     float gimbal_pitch_deg,
//     float gimbal_yaw_deg,
//     Mission::MissionItem::CameraAction camera_action)
// {
//     Mission::MissionItem new_item{};
//     new_item.latitude_deg = latitude_deg;
//     new_item.longitude_deg = longitude_deg;
//     new_item.relative_altitude_m = relative_altitude_m;
//     new_item.speed_m_s = speed_m_s;
//     new_item.is_fly_through = is_fly_through;
//     new_item.gimbal_pitch_deg = gimbal_pitch_deg;
//     new_item.gimbal_yaw_deg = gimbal_yaw_deg;
//     new_item.camera_action = camera_action;
//     return new_item;
// }

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

    std::cout << "System ready" << std::endl;
    std::cout << "Creating and uploading mission" << std::endl;
    auto mission = std::make_shared<Mission>(system);

    std::vector<Mission::MissionItem> mission_items;

    // mission_items.push_back(make_mission_item(
    //     47.398170327054473,
    //     8.5456490218639658,
    //     10.0f,
    //     5.0f,
    //     false,
    //     0.0f,
    //     0.0f,
    //     Mission::MissionItem::CameraAction::None));

    // mission_items.push_back(make_mission_item(
    //     47.398241338125118,
    //     8.5455360114574432,
    //     10.0f,
    //     2.0f,
    //     true,
    //     0.0f,
    //     -60.0f,
    //     Mission::MissionItem::CameraAction::TakePhoto));

    // mission_items.push_back(make_mission_item(
    //     47.398139363821485,
    //     8.5453846156597137,
    //     10.0f,
    //     5.0f,
    //     true,
    //     -45.0f,
    //     0.0f,
    //     Mission::MissionItem::CameraAction::StartVideo));

    // mission_items.push_back(make_mission_item(
    //     47.398058617228855,
    //     8.5454618036746979,
    //     10.0f,
    //     2.0f,
    //     false,
    //     -90.0f,
    //     30.0f,
    //     Mission::MissionItem::CameraAction::StopVideo));

    // mission_items.push_back(make_mission_item(
    //     47.398100366082858,
    //     8.5456969141960144,
    //     10.0f,
    //     5.0f,
    //     false,
    //     -45.0f,
    //     -30.0f,
    //     Mission::MissionItem::CameraAction::StartPhotoInterval));

    // mission_items.push_back(make_mission_item(
    //     47.398001890458097,
    //     8.5455576181411743,
    //     10.0f,
    //     5.0f,
    //     false,
    //     0.0f,
    //     0.0f,
    //     Mission::MissionItem::CameraAction::StopPhotoInterval));

    Mission::MissionItem mission_item;
    mission_item.latitude_deg = 47.398170327054473;     // range: -90 to +90
    mission_item.longitude_deg = 8.5456490218639658;    // range: -180 to +180
    mission_item.relative_altitude_m = 10.0f;           // takeoff altitude
    mission_item.speed_m_s = 5.0f;
    mission_item.is_fly_through = false;                // stop on the waypoint
    mission_item.gimbal_pitch_deg = 20.0f;
    mission_item.gimbal_yaw_deg = 60.0f;
    mission_item.camera_action = Mission::MissionItem::CameraAction::TakePhoto; // action to trigger
    mission_items.push_back(mission_item);

    mission_item.latitude_deg = 47.398139363821485;
    mission_item.longitude_deg = 8.5453846156597137;
    mission_item.relative_altitude_m = 10.0f;
    mission_item.speed_m_s = 5.0f;
    mission_item.is_fly_through = true;
    mission_item.gimbal_pitch_deg = -45.0f;
    mission_item.gimbal_yaw_deg = 0.0f;
    mission_item.camera_action = Mission::MissionItem::CameraAction::StartVideo;
    mission_items.push_back(mission_item);

    {
        std::cout << "Uploading mission..." << std::endl;
        // We only have the upload_mission function asynchronous for now, so we wrap it using
        // std::future.
        auto prom = std::make_shared<std::promise<Mission::Result>>();
        auto future_result = prom->get_future();
        Mission::MissionPlan mission_plan;
        mission_plan.mission_items = mission_items;
        mission->upload_mission_async(
            mission_plan, [prom](Mission::Result result) { prom->set_value(result); });

        const Mission::Result result = future_result.get();
        if (result != Mission::Result::Success) {
            std::cout << "Mission upload failed (" << result << "), exiting." << std::endl;
            return 1;
        }
        std::cout << "Mission uploaded." << std::endl;

    }

    // Arm vehicle
    auto action = std::make_shared<Action>(system);
    std::cout << "Arming..." << std::endl;
    const Action::Result arm_result = action->arm();

    if (arm_result != Action::Result::Success) {
        std::cout << "Arming failed:" << arm_result << std::endl;
        return 1;
    }
    std::cout << "Armed." << std::endl;

    std::atomic<bool> want_to_pause{false};
    // Before starting the mission, we want to be sure to subscribe to the mission progress.
    mission->subscribe_mission_progress(
        [&want_to_pause](Mission::MissionProgress mission_progress) {
            std::cout << "Mission status update: " << mission_progress.current << " / "
                      << mission_progress.total << std::endl;

            if (mission_progress.current >= 2) {
                // We can only set a flag here. If we do more request inside the callback,
                // we risk blocking the system.
                want_to_pause = true;
            }
        });

    {
        std::cout << "Starting mission." << std::endl;
        auto prom = std::make_shared<std::promise<Mission::Result>>();
        auto future_result = prom->get_future();
        mission->start_mission_async([prom](Mission::Result result) {
            prom->set_value(result);
            std::cout << "Started mission." << std::endl;
        });

        const Mission::Result result = future_result.get();
        if (result != Mission::Result::Success) {
            std::cerr << "Mission start failed: " << result << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // while (!want_to_pause) {
    //     std::cout << "risk blocking the system with uploading mission." << std::endl;
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    // }

    // {
    //     auto prom = std::make_shared<std::promise<Mission::Result>>();
    //     auto future_result = prom->get_future();

    //     std::cout << "Pausing mission..." << std::endl;
    //     mission->pause_mission_async([prom](Mission::Result result) { prom->set_value(result);
    //     });

    //     const Mission::Result result = future_result.get();
    //     if (result != Mission::Result::Success) {
    //         std::cout << "Failed to pause mission (" << result << ")" << std::endl;
    //     } else {
    //         std::cout << "Mission paused." << std::endl;
    //     }
    // }

    // Pause for 5 seconds.
    // std::this_thread::sleep_for(std::chrono::seconds(5));

    // Then continue.
    // {
    //     auto prom = std::make_shared<std::promise<Mission::Result>>();
    //     auto future_result = prom->get_future();

    //     std::cout << "Resuming mission..." << std::endl;
    //     mission->start_mission_async([prom](Mission::Result result) { prom->set_value(result);
    //     });

    //     const Mission::Result result = future_result.get();
    //     if (result != Mission::Result::Success) {
    //         std::cout << "Failed to resume mission (" << result << ")" << std::endl;
    //     } else {
    //         std::cout << "Resumed mission." << std::endl;
    //     }
    // }

    while (!mission->is_mission_finished().second) {
        std::cout << "Not finished mission." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Take off
    // std::cout << "Taking off..." << std::endl;
    // const Action::Result takeoff_result = action->takeoff();
    // if (takeoff_result != Action::Result::Success) {
    //     std::cout << "Takeoff failed:" << takeoff_result << std::endl;
    //     return 1;
    // }

    // Let it hover for a bit before landing again.
    // std::this_thread::sleep_for(std::chrono::seconds(10));

    {
        // We are done, and can do RTL to go home.
        std::cout << "Commanding RTL..." << std::endl;
        const Action::Result result = action->return_to_launch();
        if (result != Action::Result::Success) {
            std::cout << "Failed to command RTL (" << result << ")" << std::endl;
        } else {
            std::cout << "Commanded RTL." << std::endl;
        }
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

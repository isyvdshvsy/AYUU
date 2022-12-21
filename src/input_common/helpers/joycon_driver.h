// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <mutex>
#include <span>
#include <thread>

#include "input_common/helpers/joycon_protocol/calibration.h"
#include "input_common/helpers/joycon_protocol/generic_functions.h"
#include "input_common/helpers/joycon_protocol/joycon_types.h"
#include "input_common/helpers/joycon_protocol/poller.h"
#include "input_common/helpers/joycon_protocol/rumble.h"

namespace InputCommon::Joycon {

class JoyconDriver final {
public:
    explicit JoyconDriver(std::size_t port_);

    ~JoyconDriver();

    DriverResult RequestDeviceAccess(SDL_hid_device_info* device_info);
    DriverResult InitializeDevice();
    void Stop();

    bool IsConnected() const;
    bool IsVibrationEnabled() const;

    FirmwareVersion GetDeviceVersion() const;
    Color GetDeviceColor() const;
    std::size_t GetDevicePort() const;
    ControllerType GetDeviceType() const;
    ControllerType GetHandleDeviceType() const;
    SerialNumber GetSerialNumber() const;
    SerialNumber GetHandleSerialNumber() const;

    DriverResult SetVibration(const VibrationValue& vibration);
    DriverResult SetLedConfig(u8 led_pattern);
    DriverResult SetPasiveMode();
    DriverResult SetActiveMode();
    DriverResult SetNfcMode();
    DriverResult SetRingConMode();

    void SetCallbacks(const Joycon::JoyconCallbacks& callbacks);

    // Returns device type from hidapi handle
    static Joycon::DriverResult GetDeviceType(SDL_hid_device_info* device_info,
                                              Joycon::ControllerType& controller_type);

    // Returns serial number from hidapi handle
    static Joycon::DriverResult GetSerialNumber(SDL_hid_device_info* device_info,
                                                Joycon::SerialNumber& serial_number);

private:
    struct SupportedFeatures {
        bool passive{};
        bool hidbus{};
        bool irs{};
        bool motion{};
        bool nfc{};
        bool vibration{};
    };

    /// Main thread, actively request new data from the handle
    void InputThread(std::stop_token stop_token);

    /// Called everytime a valid package arrives
    void OnNewData(std::span<u8> buffer);

    /// Updates device configuration to enable or disable features
    void SetPollingMode();

    /// Returns true if input thread is valid and doesn't need to be stopped
    bool IsInputThreadValid() const;

    /// Returns true if the data should be interpreted. Otherwise the error counter is incremented
    bool IsPayloadCorrect(int status, std::span<const u8> buffer);

    /// Returns a list of supported features that can be enabled on this device
    SupportedFeatures GetSupportedFeatures();

    // Protocol Features
    std::unique_ptr<CalibrationProtocol> calibration_protocol = nullptr;
    std::unique_ptr<GenericProtocol> generic_protocol = nullptr;
    std::unique_ptr<JoyconPoller> joycon_poller = nullptr;
    std::unique_ptr<RumbleProtocol> rumble_protocol = nullptr;

    // Connection status
    bool is_connected{};
    u64 delta_time;
    std::size_t error_counter{};
    std::shared_ptr<JoyconHandle> hidapi_handle = nullptr;
    std::chrono::time_point<std::chrono::steady_clock> last_update;

    // External device status
    bool starlink_connected{};
    bool ring_connected{};
    bool amiibo_detected{};

    // Harware configuration
    u8 leds{};
    ReportMode mode{};
    bool passive_enabled{};   // Low power mode, Ideal for multiple controllers at the same time
    bool hidbus_enabled{};    // External device support
    bool irs_enabled{};       // Infrared camera input
    bool motion_enabled{};    // Enables motion input
    bool nfc_enabled{};       // Enables Amiibo detection
    bool vibration_enabled{}; // Allows vibrations

    // Calibration data
    GyroSensitivity gyro_sensitivity{};
    GyroPerformance gyro_performance{};
    AccelerometerSensitivity accelerometer_sensitivity{};
    AccelerometerPerformance accelerometer_performance{};
    JoyStickCalibration left_stick_calibration{};
    JoyStickCalibration right_stick_calibration{};
    MotionCalibration motion_calibration{};

    // Fixed joycon info
    FirmwareVersion version{};
    Color color{};
    std::size_t port{};
    ControllerType device_type{};        // Device type reported by controller
    ControllerType handle_device_type{}; // Device type reported by hidapi
    SerialNumber serial_number{};        // Serial number reported by controller
    SerialNumber handle_serial_number{}; // Serial number type reported by hidapi
    SupportedFeatures supported_features{};

    // Thread related
    mutable std::mutex mutex;
    std::jthread input_thread;
    bool input_thread_running{};
    bool disable_input_thread{};
};

} // namespace InputCommon::Joycon

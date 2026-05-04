#pragma once

#include "config/ISettingsStore.hpp"
#include <functional>

class Config {
public:
    // Callbacks return true if a reboot is required to fully apply the change.
    using NetworkCb = std::function<bool(const Settings::Network&)>;
    using BeaconCb  = std::function<bool(const Settings::Beacon&)>;
    using DisplayCb = std::function<bool(const Settings::Display&)>;

    explicit Config(ISettingsStore& store);

    bool            load();
    const Settings& get() const;

    // Apply a full or partial settings update.
    // Compares each category against the current settings, fires callbacks only
    // for categories that changed, accumulates reboot flags, then persists.
    // Returns false if saving to the store failed.
    bool apply(const Settings& updated, bool& rebootNeeded);

    void onNetworkChanged(NetworkCb cb);
    void onBeaconChanged(BeaconCb cb);
    void onDisplayChanged(DisplayCb cb);

private:
    ISettingsStore& _store;
    Settings        _settings;

    NetworkCb _networkCb;
    BeaconCb  _beaconCb;
    DisplayCb _displayCb;
};

#pragma once
#include "interfaces.h"

class NvsConfig : public IConfig {
public:
    NvsConfig();
    bool               load()                    override;
    bool               save()                    override;
    const DeviceConfig& get()              const  override;
    void               set(const DeviceConfig&)   override;

private:
    DeviceConfig m_cfg;

    static bool readStr(const char* ns, const char* key,
                        char* out, size_t maxLen);
    static bool writeStr(const char* ns, const char* key, const char* val);
    static bool readU8(const char* ns, const char* key, uint8_t& out);
    static bool writeU8(const char* ns, const char* key, uint8_t val);

    static constexpr const char* NS = "cyd_cfg";
};

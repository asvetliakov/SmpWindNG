#include "Config.h"

using namespace RE;
using namespace RE::BSScript;
using namespace REL;
using namespace SKSE;

namespace wind {
    extern Config g_config;
    extern Config g_configDefault;

    static bool getBool(StaticFunctionTag*, int32_t id) {
        if (id >= 0 && id < Config::BOOL_COUNT) {
            return g_config.getb(id);
        } else {
            return false;
        }
    }

    static bool getBoolDefault(StaticFunctionTag*, int32_t id) {
        if (id >= 0 && id < Config::BOOL_COUNT) {
            return g_configDefault.getb(id);
        } else {
            return false;
        }
    }

    static void setBool(StaticFunctionTag*, int32_t id, bool b) {
        if (id >= 0 && id < Config::BOOL_COUNT) {
            g_config.set(id, b);
        }
    }

    static float getFloat(StaticFunctionTag*, int32_t id) {
        if (id >= 0 && id < Config::FLOAT_COUNT) {
            return g_config.getf(id);
        } else {
            return 0.0f;
        }
    }

    static float getFloatDefault(StaticFunctionTag*, int32_t id) {
        if (id >= 0 && id < Config::FLOAT_COUNT) {
            return g_configDefault.getf(id);
        } else {
            return 0.0f;
        }
    }

    static void setFloat(StaticFunctionTag*, int32_t id, float f) {
        if (id >= 0 && id < Config::FLOAT_COUNT) {
            g_config.set(id, f);
        }
    }

    static int32_t getInt(StaticFunctionTag*, int32_t id) {
        if (id >= 0 && id < Config::INT_COUNT) {
            return g_config.geti(id);
        } else {
            return 0;
        }
    }

    static int32_t getIntDefault(StaticFunctionTag*, int32_t id) {
        if (id >= 0 && id < Config::INT_COUNT) {
            return g_configDefault.geti(id);
        } else {
            return 0;
        }
    }

    static void setInt(StaticFunctionTag*, int32_t id, int32_t i) {
        if (id >= 0 && id < Config::INT_COUNT) {
            g_config.set(id, static_cast<int>(i));
        }
    }

    bool registerFunctions(IVirtualMachine* vmcr) {
        assert(vmcr);

        vmcr->RegisterFunction("GetBool", "JGWD_MCM", getBool);
        vmcr->RegisterFunction("GetBoolDefault", "JGWD_MCM", getBoolDefault);
        vmcr->RegisterFunction("SetBool", "JGWD_MCM", setBool);
        vmcr->RegisterFunction("GetFloat", "JGWD_MCM", getFloat);
        vmcr->RegisterFunction("GetFloatDefault", "JGWD_MCM", getFloatDefault);
        vmcr->RegisterFunction("SetFloat", "JGWD_MCM", setFloat);
        vmcr->RegisterFunction("GetInt", "JGWD_MCM", getInt);
        vmcr->RegisterFunction("GetIntDefault", "JGWD_MCM", getIntDefault);
        vmcr->RegisterFunction("SetInt", "JGWD_MCM", setInt);

        return true;
    }
}  // namespace wind
#include "napi/native_api.h"
#include "libohoskamel_api.h"

#include <string>
#include <vector>

#include <hilog/log.h>

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0x0000
#define LOG_TAG "KamelNapi"

static std::string JsToUtf8(napi_env env, napi_value value) {
    size_t length = 0;
    if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok) {
        return {};
    }
    std::vector<char> buf(length + 1);
    size_t copied = 0;
    napi_get_value_string_utf8(env, value, buf.data(), buf.size(), &copied);
    return std::string(buf.data(), copied);
}

static void DisposeKnString(const char *raw) {
    if (raw == nullptr) {
        return;
    }
    libohoskamel_ExportedSymbols *symbols = libohoskamel_symbols();
    if (symbols != nullptr && symbols->DisposeString != nullptr) {
        symbols->DisposeString(raw);
    }
}

static napi_value NapiKamelInit(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1) {
        napi_throw_error(env, nullptr, "kamelInit requires a JSON string");
        return nullptr;
    }

    napi_valuetype type = napi_undefined;
    napi_typeof(env, args[0], &type);
    if (type != napi_string) {
        napi_throw_type_error(env, nullptr, "kamelInit argument must be a string");
        return nullptr;
    }

    const std::string config = JsToUtf8(env, args[0]);
    OH_LOG_INFO(LOG_APP, "kamelInit config bytes=%{public}zu", config.size());

    const char *raw = KamelInit(config.c_str());
    if (raw == nullptr) {
        napi_throw_error(env, nullptr, "KamelInit returned null");
        return nullptr;
    }

    napi_value out = nullptr;
    napi_status st = napi_create_string_utf8(env, raw, NAPI_AUTO_LENGTH, &out);
    DisposeKnString(raw);
    if (st != napi_ok) {
        napi_throw_error(env, nullptr, "failed to create JS string from KamelInit");
        return nullptr;
    }
    return out;
}

static napi_value NapiKamelLoad(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1) {
        napi_throw_error(env, nullptr, "kamelLoad requires a JSON string");
        return nullptr;
    }

    napi_valuetype type = napi_undefined;
    napi_typeof(env, args[0], &type);
    if (type != napi_string) {
        napi_throw_type_error(env, nullptr, "kamelLoad argument must be a string");
        return nullptr;
    }

    const std::string request = JsToUtf8(env, args[0]);
    OH_LOG_INFO(LOG_APP, "kamelLoad request bytes=%{public}zu", request.size());

    const char *raw = KamelLoad(request.c_str());
    if (raw == nullptr) {
        napi_throw_error(env, nullptr, "KamelLoad returned null");
        return nullptr;
    }

    napi_value out = nullptr;
    napi_status st = napi_create_string_utf8(env, raw, NAPI_AUTO_LENGTH, &out);
    DisposeKnString(raw);
    if (st != napi_ok) {
        napi_throw_error(env, nullptr, "failed to create JS string from KamelLoad");
        return nullptr;
    }
    return out;
}

static napi_value NapiKamelCacheInfo(napi_env env, napi_callback_info info) {
    (void)info;
    OH_LOG_INFO(LOG_APP, "kamelCacheInfo");

    const char *raw = KamelCacheInfo();
    if (raw == nullptr) {
        napi_throw_error(env, nullptr, "KamelCacheInfo returned null");
        return nullptr;
    }

    napi_value out = nullptr;
    napi_status st = napi_create_string_utf8(env, raw, NAPI_AUTO_LENGTH, &out);
    DisposeKnString(raw);
    if (st != napi_ok) {
        napi_throw_error(env, nullptr, "failed to create JS string from KamelCacheInfo");
        return nullptr;
    }
    return out;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"kamelInit", nullptr, NapiKamelInit, nullptr, nullptr, nullptr, static_cast<napi_property_attributes>(napi_default | napi_enumerable), nullptr},
        {"kamelLoad", nullptr, NapiKamelLoad, nullptr, nullptr, nullptr, static_cast<napi_property_attributes>(napi_default | napi_enumerable), nullptr},
        {"kamelCacheInfo", nullptr, NapiKamelCacheInfo, nullptr, nullptr, nullptr, static_cast<napi_property_attributes>(napi_default | napi_enumerable), nullptr}};
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module kamelNapiModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "kamel_napi",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterKamelNapiModule(void) {
    napi_module_register(&kamelNapiModule);
}

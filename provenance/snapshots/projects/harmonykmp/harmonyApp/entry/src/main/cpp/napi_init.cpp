#include "libkn_api.h"
#include "napi/native_api.h"
#include "hilog/log.h"
#include <rawfile/raw_file_manager.h>
#include <dlfcn.h>
#include <string>
#include <vector>

// 避免工程侧未定义 LOG_DOMAIN 时编译失败
#ifndef LOG_DOMAIN
#define LOG_DOMAIN 0x0000
#endif

extern "C" void initResourceManager(void* resourceManager);
extern "C" void sendBoxImagePath(void* path);
extern "C" void sendImagePathList(void* paths);
extern "C" void register_kotlin_call_arkts_controller(void* controller);
extern "C" void* arkts_call_kotlin_cstr(int input);
extern "C" void arkts_free_cstr(void* ptr);

static napi_value MainArkUIViewController(napi_env env, napi_callback_info info) {
    return reinterpret_cast<napi_value>(MainArkUIViewController(env));
}

static napi_value InitResourceManagerController(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_type_error(env, nullptr, "Expected resourceManager argument");
        return nullptr;
    }

    auto manager = OH_ResourceManager_InitNativeResourceManager(env, args[0]);
    initResourceManager(manager);

    napi_value result;
    napi_create_int32(env, 0, &result);
    return result;
}

static napi_value sendImagePathListController(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_type_error(env, nullptr, "Expected paths argument");
        return nullptr;
    }

    napi_valuetype argType;
    napi_typeof(env, args[0], &argType);

    std::string joined;
    if (argType == napi_string) {
        size_t strLen = 0;
        napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
        std::vector<char> buffer(strLen + 1);
        napi_get_value_string_utf8(env, args[0], buffer.data(), buffer.size(), &strLen);
        joined.assign(buffer.data(), strLen);
    } else {
        bool isArray = false;
        napi_is_array(env, args[0], &isArray);
        if (!isArray) {
            napi_throw_type_error(env, nullptr, "Expected string[] or string");
            return nullptr;
        }

        uint32_t length = 0;
        napi_get_array_length(env, args[0], &length);
        for (uint32_t i = 0; i < length; ++i) {
            napi_value item;
            napi_get_element(env, args[0], i, &item);

            size_t strLen = 0;
            napi_get_value_string_utf8(env, item, nullptr, 0, &strLen);
            std::vector<char> buffer(strLen + 1);
            napi_get_value_string_utf8(env, item, buffer.data(), buffer.size(), &strLen);

            if (!joined.empty()) {
                joined.push_back('\n');
            }
            joined.append(buffer.data(), strLen);
        }
    }

    sendImagePathList(const_cast<char*>(joined.c_str()));
    return nullptr;
}

static napi_value sendBoxImagePathController(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_type_error(env, nullptr, "Expected path argument");
        return nullptr;
    }

    size_t strLen = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
    std::vector<char> buffer(strLen + 1);
    napi_get_value_string_utf8(env, args[0], buffer.data(), buffer.size(), &strLen);

    sendBoxImagePath(const_cast<char*>(buffer.data()));
    return nullptr;
}

static napi_value sendKotlinCallArkTsController(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1 || args[0] == nullptr) {
        napi_throw_type_error(env, nullptr, "Expected controller argument");
        return nullptr;
    }

    register_kotlin_call_arkts_controller(reinterpret_cast<void*>(args[0]));

    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value ArkTsCallKotlin(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    int32_t input = 123;
    if (argc >= 1) {
        napi_valuetype argType = napi_undefined;
        napi_typeof(env, args[0], &argType);
        if (argType == napi_number) {
            napi_get_value_int32(env, args[0], &input);
        }
    }

    void* p = arkts_call_kotlin_cstr(input);
    napi_value jsStr = nullptr;
    if (p != nullptr) {
        const char* cstr = reinterpret_cast<const char*>(p);
        napi_create_string_utf8(env, cstr, NAPI_AUTO_LENGTH, &jsStr);
        arkts_free_cstr(p);
    } else {
        napi_get_undefined(env, &jsStr);
    }

    return jsStr;
}


static napi_value nativeCreateRootNode(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    return reinterpret_cast<napi_value>(createRootRenderNode());
}

static napi_value nativeOHRenderNodeDraw(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    napi_status status;

    status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (status != napi_ok || argc < 2) {
        napi_throw_type_error(env, nullptr, "Expected canvas argument");
        return nullptr;
    }
    renderNodeDraw(env, argv[0], argv[1]);
    return nullptr;
}

static napi_value nativeOHRenderNotifyRedraw(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_status status;

    status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (status != napi_ok || argc < 1) {
        napi_throw_type_error(env, nullptr, "Expected renderNode argument");
        return nullptr;
    }
    renderNodeNotifyRedraw(env, argv[0]);
    return nullptr;
}

static napi_value nativeRegisterNodeConstructor(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1) {
        return nullptr;
    }
    setNodeConstructor(env, argv[0]);
    return nullptr;
}

static napi_value nativeSetPixelRatio(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    double ratio = 1.0;
    napi_get_value_double(env, argv[0], &ratio);
    setPixelRatio(ratio);
    return nullptr;
}

static napi_value nativeRegisterNodeStatusModifyConstructor(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1) {
        return nullptr;
    }
    buildInstance(env, argv[0]);
    return nullptr;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    androidx_compose_ui_arkui_init(env, exports);
    napi_property_descriptor desc[] = {
        {"MainArkUIViewController", nullptr, MainArkUIViewController, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"initResourceManager", nullptr, InitResourceManagerController, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendBoxImagePath", nullptr, sendBoxImagePathController, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendImagePathList", nullptr, sendImagePathListController, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendKotlinCallArkTsController", nullptr, sendKotlinCallArkTsController, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"arkTsCallKotlin", nullptr, ArkTsCallKotlin, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"nativeCreateRootNode", nullptr, nativeCreateRootNode, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"nativeOHRenderNodeDraw", nullptr, nativeOHRenderNodeDraw, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"nativeOHRenderNotifyRedraw", nullptr, nativeOHRenderNotifyRedraw, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"nativeRegisterNodeConstructor", nullptr, nativeRegisterNodeConstructor, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"nativeRegisterNodeStatusModifyConstructor", nullptr, nativeRegisterNodeStatusModifyConstructor, nullptr, nullptr, nullptr, napi_default, nullptr },
        {"nativeSetPixelRatio", nullptr, nativeSetPixelRatio, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&demoModule);
}

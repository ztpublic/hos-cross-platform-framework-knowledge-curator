#include "napi/native_api.h"
#include "libkotlinxio_api.h"
#include <cmath>
#include <limits>

static napi_value ReadText(napi_env env, const char* text) {
  if (text == nullptr) {
    napi_throw_error(env, nullptr, "Native KOTLINX-IO returned null");
    return nullptr;
  }
  napi_value result = nullptr;
  if (napi_create_string_utf8(env, text, NAPI_AUTO_LENGTH, &result) != napi_ok) {
    napi_throw_error(env, nullptr, "Unable to create native KOTLINX-IO text");
    return nullptr;
  }
  return result;
}

static bool ReadIndex(napi_env env, napi_value value, int32_t& output) {
  double number = 0;
  if (napi_get_value_double(env, value, &number) != napi_ok || !std::isfinite(number) ||
      std::trunc(number) != number || number < 0 || number > std::numeric_limits<int32_t>::max()) {
    return false;
  }
  output = static_cast<int32_t>(number);
  return true;
}

static napi_value Catalog(napi_env env, napi_callback_info) {
  const auto text = IoCatalog();
  auto result = ReadText(env, reinterpret_cast<const char*>(text));
  IoFree(text);
  return result;
}

static napi_value GetIo(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  if (napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) != napi_ok || argc < 1) {
    napi_throw_type_error(env, nullptr, "getExample requires an index");
    return nullptr;
  }
  int32_t index = 0;
  if (!ReadIndex(env, args[0], index)) {
    napi_throw_type_error(env, nullptr, "getExample index must be a non-negative integer");
    return nullptr;
  }
  const auto text = IoGet(index);
  auto result = ReadText(env, reinterpret_cast<const char*>(text));
  IoFree(text);
  return result;
}

static napi_value CurrentIo(napi_env env, napi_callback_info) {
  const auto text = IoCurrent();
  auto result = ReadText(env, reinterpret_cast<const char*>(text));
  IoFree(text);
  return result;
}

static napi_value RunChecks(napi_env env, napi_callback_info) {
  const auto text = IoRunChecks();
  auto result = ReadText(env, reinterpret_cast<const char*>(text));
  IoFree(text);
  return result;
}

static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor methods[] = {
      {"getCatalog", nullptr, Catalog, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"getExample", nullptr, GetIo, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"currentIo", nullptr, CurrentIo, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"runChecks", nullptr, RunChecks, nullptr, nullptr, nullptr, napi_default, nullptr},
  };
  if (napi_define_properties(env, exports, 4, methods) != napi_ok) {
    napi_throw_error(env, nullptr, "Unable to register KOTLINX-IO exports");
    return nullptr;
  }
  return exports;
}

static napi_module kotlinxioModule = {1, 0, nullptr, Init, "entry", nullptr, {0}};

extern "C" __attribute__((constructor)) void RegisterIoModule() {
  napi_module_register(&kotlinxioModule);
}

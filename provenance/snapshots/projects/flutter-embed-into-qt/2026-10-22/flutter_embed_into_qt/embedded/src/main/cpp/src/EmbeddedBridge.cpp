/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <QDebug>
#include <QString>
#include <QMetaObject>
#include "EmbeddedBridge.h"
#include "EmbeddedBridgeJsSide.h"
#include "EmbeddedBridgeQtSide.h"
#include "napi_common.h"
#include "napi_util.h"
#include <arkui/native_node_napi.h>
#include <cstdint>


namespace EB {

napi_env EmbeddedBridge::g_env = nullptr;
napi_ref EmbeddedBridge::g_ref_napi_obj = nullptr;
napi_threadsafe_function EmbeddedBridge::g_cb_ref;

const std::string EmbeddedBridgeTAG = "[EmbeddedBridge] ";
const int ARG_INDEX_HANDLER = 0;
const int ARG_INDEX_FUNCTION_NAME = 1;
const int ARG_INDEX_DATA = 2;

napi_value EmbeddedBridge::dispatchMessage(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value args[3] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
    std::string handler, funName, data;
    GetString(env, args[ARG_INDEX_HANDLER], handler);
    GetString(env, args[ARG_INDEX_FUNCTION_NAME], funName);
    GetString(env, args[ARG_INDEX_DATA], data);
    QMetaObject::invokeMethod(EmbeddedBridgeJsSide::instance(), "sigSendData", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(handler)),
                              Q_ARG(QString, QString::fromStdString(funName)),
                              Q_ARG(QString, QString::fromStdString(data)));
    return nullptr;
}

// 发起向JS调用时的回调
static void CallBackFunction(napi_env env, napi_value jsCallback, void *context, void *data)
{
    size_t argc = 3;
    napi_value argv[3];
    napi_value out;
    // data是napi_call_threadsafe_function传入的参数
    JsCbParam *param = static_cast<JsCbParam *>(data);
    napi_create_string_utf8(env, param->handler.c_str(), NAPI_AUTO_LENGTH, &argv[ARG_INDEX_HANDLER]);
    napi_create_string_utf8(env, param->fun.c_str(), NAPI_AUTO_LENGTH, &argv[ARG_INDEX_FUNCTION_NAME]);
    napi_create_string_utf8(env, param->data.c_str(), NAPI_AUTO_LENGTH, &argv[ARG_INDEX_DATA]);
    napi_call_function(env, nullptr, jsCallback, argc, argv, &out);
    delete param;
}

//销毁napi_threadsafe_function时的回调
static void FinalizeFunction(napi_env env, void* data, void* hint)
{
    qInfo() << EmbeddedBridgeTAG.c_str() << "FinalizeFunction";
}
/*
  ETS向C侧注册线程安全方法
 */
napi_value EmbeddedBridge::registerThreadSafeFunc(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = {NULL};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
    std::string idStr;
    GetString(env, args[0], idStr);
    napi_value id;
    NAPI_CALL(env, napi_create_string_utf8(env, idStr.c_str(), NAPI_AUTO_LENGTH, &id));
    if (EmbeddedBridge::g_cb_ref) {
        napi_release_threadsafe_function(EmbeddedBridge::g_cb_ref, napi_tsfn_release);
    }
    NAPI_CALL(env, napi_create_threadsafe_function(env, args[1], nullptr, id, 0, 1, nullptr,
        FinalizeFunction, nullptr, CallBackFunction, &EmbeddedBridge::g_cb_ref));
    return nullptr;
}

void EmbeddedBridge::invokeMessage(const QString &handler, const QString &fun, const QString &data)
{
    //QT向ArkTS发送消息
    qInfo() << EmbeddedBridgeTAG.c_str() << "invokeMessage handler:"<<handler << " fun:"<< fun << " data:"<< data;
    JsCbParam *param = new JsCbParam();
    param->handler = handler.toStdString();
    param->fun = fun.toStdString();
    param->data = data.toStdString();
    napi_call_threadsafe_function(EmbeddedBridge::g_cb_ref, param, napi_tsfn_nonblocking);
}

} // namespace EB
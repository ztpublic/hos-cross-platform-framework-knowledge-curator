#include "conference_process_impl_ohos.h"
#include "conference_process.h"
#include "log_stream.h"
#include "QtNapi/napi.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QWidget>
#include <QScreen>

constexpr char kBundleName[] = "com.dingtalk.hmos.pc.demo";
constexpr char kModuleName[] = "meeting";
constexpr char kAbilityName[] = "MeetingAbility";

constexpr char kSystemScreenCapturePermission[] = "ohos.permission.CUSTOM_SCREEN_CAPTURE";
constexpr char kSystemMicphonePermission[] = "ohos.permission.MICROPHONE";
constexpr char kSystemCameraPermission[] = "ohos.permission.CAMERA";

namespace {
namespace ErrorCode {
constexpr int64_t GetPermissonFail = -1;
constexpr int64_t StartAbilityUnknownResult = -2;
}  // namespace ErrorCode
}  // namespace

namespace {
class QAbilityResultReceiverImpl : public QOpenHarmonyAbility::QAbilityResultReceiver {
 public:
  QAbilityResultReceiverImpl(
      const std::function<void()>& on_success,
      const std::function<void(int64_t, const std::string&)>& on_failure)
      : on_success_(on_success)
      , on_failure_(on_failure){};

  virtual ~QAbilityResultReceiverImpl(){};

  void handleResult(const Napi::Value& err, const Napi::Value& result = Napi::Value()) override {
    if (!err.IsObject()) {
      LOG(ERROR) << "[QAbilityResultReceiverImpl] handleResult, err is not object.";
      if (on_failure_) {
        on_failure_(-1, "Start Ability fail, invalid result.");
      }
      return;
    }

    const int32_t code = err.As<Napi::Object>().Get("code").ToNumber().Int32Value();

    if (code == 0) {
      if (on_success_) {
        on_success_();
      }
    } else {
      if (on_failure_) {
        on_failure_(code, "Start Ability fail.");
      }
    }
  }

 private:
  std::function<void()> on_success_;
  std::function<void(int64_t, const std::string&)> on_failure_;
};

}  // namespace

std::shared_ptr<ConferenceProcess> ConferenceProcess::GetInstance() {
  static std::shared_ptr<ConferenceProcessImpl> instance = std::make_shared<ConferenceProcessImpl>();
  return instance;
}

ConferenceProcessImpl::ConferenceProcessImpl() {
  permissions_ << kSystemScreenCapturePermission;
  permissions_ << kSystemMicphonePermission;
  permissions_ << kSystemCameraPermission;
}

void ConferenceProcessImpl::Run(
    const std::string& params,
    const std::function<void()>& on_success,
    const std::function<void(int64_t, const std::string&)>& on_failure) {
  std::weak_ptr<ConferenceProcessImpl> weak_this = shared_from_this();
  auto on_get_permission_success = [weak_this, params, on_success, on_failure]() {
    auto shared_this = weak_this.lock();    
    if (shared_this) {
      shared_this->StartAbility(params, on_success, on_failure);
    }
  };

  auto on_get_permission_failure = [on_failure](int64_t code, std::string reason) {
    if (on_failure) {
      on_failure(code, reason);
    }
  };

  GetMeetingPermissionAsync(on_get_permission_success,
                            on_get_permission_failure);
}

void ConferenceProcessImpl::StartAbility(
    const std::string& params,
    const std::function<void()>& on_success,
    const std::function<void(int64_t, const std::string&)>& on_failure) {
  QOpenHarmonyWant* want = new QOpenHarmonyWant;
  want->bundleName = QString(kBundleName);
  want->moduleName = QString(kModuleName);
  want->abilityName = QString(kAbilityName);
  want->parameters["launchParam"] = QString::fromStdString(params);
  want->flags = 0;

  if (start_ability_callback_) {
    delete start_ability_callback_;
  }
  start_ability_callback_ = new QAbilityResultReceiverImpl(on_success,
                                                           on_failure);
  QOpenHarmonyStartOptions start_options;
  start_options.windowWidth = 640;
  start_options.windowHeight = 585;
  start_options.withAnimation = false;
  start_options.startWindowBackgroundColor = "#00000000";
  QDesktopWidget* desktop = QApplication::desktop();
  QRect screen = desktop->availableGeometry();
  qreal dpr = QGuiApplication::primaryScreen()->devicePixelRatio();
  int physical_width = screen.width() * dpr;
  int physical_height = screen.height() * dpr;
  start_options.windowLeft = physical_width / 2 - start_options.windowWidth / 2;
  start_options.windowTop = physical_height / 2 - start_options.windowHeight / 2;
  LOG(INFO) << "[mozi][tblive_linux] StartAbility screen wxh:" << physical_width << "x" << physical_height
            << "window wxh:" << start_options.windowWidth << "x" << start_options.windowHeight
            << "window left:" << start_options.windowLeft << "top:" << start_options.windowTop;

  QOpenHarmonyAbility::start(*want, start_options, start_ability_callback_);
}

void ConferenceProcessImpl::GetMeetingPermissionAsync(
    const std::function<void()>& on_success,
    const std::function<void(int64_t, const std::string&)>& on_failure) {
  QtOh::PermissionResult capture_result =
      QtOh::checkPermission(QString(kSystemScreenCapturePermission));
  QtOh::PermissionResult mic_result =
      QtOh::checkPermission(QString(kSystemMicphonePermission));
  QtOh::PermissionResult camera_result =
      QtOh::checkPermission(QString(kSystemCameraPermission));

  if (capture_result == QtOh::PermissionResult::Granted &&
      mic_result == QtOh::PermissionResult::Granted &&
      camera_result == QtOh::PermissionResult::Granted) {
    LOG(INFO) << "[mozi][tblive_linux] GetMeetingPermissionAsync, "
              << "get permission success.";
    if (on_success) {
      on_success();
    }
    return;
  } else {
    LOG(INFO) << "[mozi][tblive_linux] GetMeetingPermissionAsync, "
              << "get permission fail.";
    // client_ding::ViewRouter::Instance().getObject("MainView")->getView()->show();
  }

  std::weak_ptr<ConferenceProcessImpl> weak_this = shared_from_this();
  QtOh::PermissionResultCallback request_callback =
      [on_success, on_failure, weak_this](const QtOh::PermissionRequestResultEx& result_ex) {
        LOG(INFO) << "[mozi][tblive_linux] GetMeetingPermissionAsync, RequestPermissionCallback";
        auto shared_this = weak_this.lock();
        if (shared_this) {
          shared_this->RequestPermissionCallback(result_ex, on_success, on_failure, true);
        }
      };

  QtOh::requestPermissions(permissions_, request_callback);
}

void ConferenceProcessImpl::RequestPermissionCallback(
    const QtOh::PermissionRequestResultEx& result_ex,
    const std::function<void()>& on_success,
    const std::function<void(int64_t, const std::string&)>& on_failure,
    bool is_first) {
  LOG(INFO) << "[mozi][tblive_linux] RequestPermissionCallback, permissions size:"
            << result_ex.permissions.size();
  do {
    const auto index = result_ex.permissions.indexOf(QString(kSystemScreenCapturePermission));
    if (index == -1) {
      LOG(ERROR) << "[mozi][tblive_linux] GetMeetingPermissionAsync, "
                 << "get permission fail, not found permission key in result.";
      break;
    }

    if (result_ex.authResults.size() <= index ||
        result_ex.dialogShownResults.size() <= index) {
      LOG(ERROR) << "[mozi][tblive_linux] GetMeetingPermissionAsync, "
                 << "get permission fail, permission index out of result.";
      break;
    }

    if (result_ex.authResults[index] == QtOh::PermissionResult::Granted) {
      LOG(INFO) << "[mozi][tblive_linux] GetMeetingPermissionAsync, "
                << "get permission success.";
      if (on_success) {
        on_success();
        return;
      }
      break;
    } else if (result_ex.authResults[index] == QtOh::PermissionResult::Denied) {
      if (!is_first) {
        LOG(INFO) << "[mozi][tblive_linux] GetMeetingPermissionAsync, "
                  << "get permission fail, user refuse permission.";
        break;
      }

      if (result_ex.dialogShownResults[index]) {
        LOG(INFO) << "[mozi][tblive_linux] GetMeetingPermissionAsync, "
                  << "get permission fail, user refuse permission.";
        break;
      } else {
        std::weak_ptr<ConferenceProcessImpl> weak_this = shared_from_this();
        QtOh::PermissionResultCallback request_callback =
            [on_success, on_failure, weak_this](const QtOh::PermissionRequestResultEx& result_ex) {
              auto shared_this = weak_this.lock();      
              if (shared_this) {
                shared_this->RequestPermissionCallback(result_ex, on_success, on_failure, false);
              }
            };

        QtOh::requestPermissionsOnSetting(permissions_, request_callback);
        return;
      }
    } else {
      LOG(ERROR) << "[mozi][tblive_linux] GetMeetingPermissionAsync, "
                 << "get permission fail, invalid request.";
      break;
    }
  } while (true);

  if (on_failure) {
    on_failure(ErrorCode::GetPermissonFail, "Get permission fail.");
  }
}
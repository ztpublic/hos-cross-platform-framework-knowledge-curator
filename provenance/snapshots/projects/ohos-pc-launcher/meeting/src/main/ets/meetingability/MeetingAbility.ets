import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import UIAbility from '@ohos.app.ability.UIAbility';
import Window from '@ohos.window'
import Want from '@ohos.app.ability.Want';
import qpa from 'libplugins_platforms_qopenharmony.so'
import hilog from '@ohos.hilog';

type handleAbilityClose = ()=>void

export default class MeetingAbility extends UIAbility {
  tag = "QtForOpenHarmony";
  domain = 0xFF00;
  private launchApplication = "libmeeting_ohos.so"
  private launchParams = ""
  private name: string = 'qt_entry_window'
  private closeAbility: handleAbilityClose | undefined = undefined

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    let name = want.parameters?.windowName as string;
    if (name != undefined && name !== '')
      this.name = name
    let newAbility = want.parameters?.newAbility as boolean;
    if (newAbility !== undefined)
      return;
  }

  newLocalStorage() {
    return new LocalStorage()
  }

  onDestroy() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onDestroy');
  }

  onWindowStageCreate(windowStage: Window.WindowStage) {
    // Main window is created, set main page for this ability
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onWindowStageCreate');
    let localStore = new LocalStorage();
    localStore.setOrCreate('idName', this.name);
    let w = windowStage.getMainWindowSync();
    let newAbility = this.launchWant.parameters?.newAbility as boolean;

    windowStage.loadContent('pages/Index', localStore).then(()=>{
      qpa.handleJsTopWindowCreated(this.name, this);

      // let window = windowStage.getMainWindowSync();
      // window.setWindowBackgroundColor("#00000000");
      // //Note：系统未放开出来，在ohos_window.d.ts中手动添加的。
      // window.setWindowContainerColor("#00000000", "#00000000");

      if (newAbility !== undefined) {
        this.context.setMissionLabel(this.launchWant.parameters?.title as string);
        return;
      }
    })

    if (newAbility !== undefined)
      return;

    this.launchParamSet();
    qpa.startQtApplication(this);
  }

  async launchParamSet(){
    let params:string[] = [];
    if(this.launchWant.uri != undefined){
      params.push(this.launchWant.uri)
    }
    //params.push('--type=conference --tcp-port=2938 --channel=UXgGmIN --conf-launch-mode=0 --cid= --conf-and-class --conf-mode=0 --as-caller --dt-conf-studio --start-time=1734864057444 --enable-airecord --enable_colla_exclusive_service --camera-def-close --enable-dsound --audio-invert --enable-capture-ds2 --enable-detective-device-changed --enable-multi-streams --enable-holmes-log --event-toast --interrupt-status --device-output --voice-active --socketio-log --enable-vr-bg --enable-custom-vr-bg --locale=zh-CN --enable_exit_code --enable_signal_handle --enable-pstn-sip --enable_hide_unjoin_member --enable_camera_status_optimize --enable_smooth_mode_report --enable_system_info_report --enable_conf_force_report --enable_sdk_statistics_optimize --bug_fix_1017_pingtian --stop_ring_1017_pingtian --meeting_control_optimize_windows --enable_flash_meeting_conf_v210913 --enable_cloud_recorder_presenter_bug_fix --enable_ipc_gray_switch_query --enable_pc_signal_level --enable_ipc_gray_switch_query_v2 --new_frame_enable_windows --new_frame_enable_v2_windows --enable_camera_status_fix_1017 --enable_camera_open_limited --conf_concurrency_limit --enable_cell_controller_0515 --enable_conf_upgrade --enable_share_optimize_v2_windows --conf_count_down_limit_v2 --enable_conf_day_meeting --conf_toast_fix --main-orgid=439446171 --selected-org-name=钉钉 --enable_schema_open_join_conf_view --app-environment=0 --self-uid=119732102 --enable-crash --enable-wer')
    if (this.launchWant.parameters?.launchParam !== undefined){
      let cmdLine:string = this.launchWant.parameters?.launchParam as string;
      //let cmdLine:string = '--type=conference --tcp-port=2938 --channel=UXgGmIN --conf-launch-mode=0 --cid= --conf-and-class --conf-mode=0 --as-caller --dt-conf-studio --start-time=1734864057444 --enable-airecord --enable_colla_exclusive_service --camera-def-close --enable-dsound --audio-invert --enable-capture-ds2 --enable-detective-device-changed --enable-multi-streams --enable-holmes-log --event-toast --interrupt-status --device-output --voice-active --socketio-log --enable-vr-bg --enable-custom-vr-bg --locale=zh-CN --enable_exit_code --enable_signal_handle --enable-pstn-sip --enable_hide_unjoin_member --enable_camera_status_optimize --enable_smooth_mode_report --enable_system_info_report --enable_conf_force_report --enable_sdk_statistics_optimize --bug_fix_1017_pingtian --stop_ring_1017_pingtian --meeting_control_optimize_windows --enable_flash_meeting_conf_v210913 --enable_cloud_recorder_presenter_bug_fix --enable_ipc_gray_switch_query --enable_pc_signal_level --enable_ipc_gray_switch_query_v2 --new_frame_enable_windows --new_frame_enable_v2_windows --enable_camera_status_fix_1017 --enable_camera_open_limited --conf_concurrency_limit --enable_cell_controller_0515 --enable_conf_upgrade --enable_share_optimize_v2_windows --conf_count_down_limit_v2 --enable_conf_day_meeting --conf_toast_fix --main-orgid=439446171 --selected-org-name=钉钉 --enable_schema_open_join_conf_view --app-environment=0 --self-uid=119732102 --enable-crash --enable-wer';
      //let cmdLine:string = '--type=conference';
      params = cmdLine.split(' ');
    }
    //let cmdLine:string = '--type=conference --tcp-port=2938 --channel=UXgGmIN --conf-launch-mode=0 --cid= --conf-and-class --conf-mode=0 --as-caller --dt-conf-studio --start-time=1734864057444 --enable-airecord --enable_colla_exclusive_service --camera-def-close --enable-dsound --audio-invert --enable-capture-ds2 --enable-detective-device-changed --enable-multi-streams --enable-holmes-log --event-toast --interrupt-status --device-output --voice-active --socketio-log --enable-vr-bg --enable-custom-vr-bg --locale=zh-CN --enable_exit_code --enable_signal_handle --enable-pstn-sip --enable_hide_unjoin_member --enable_camera_status_optimize --enable_smooth_mode_report --enable_system_info_report --enable_conf_force_report --enable_sdk_statistics_optimize --bug_fix_1017_pingtian --stop_ring_1017_pingtian --meeting_control_optimize_windows --enable_flash_meeting_conf_v210913 --enable_cloud_recorder_presenter_bug_fix --enable_ipc_gray_switch_query --enable_pc_signal_level --enable_ipc_gray_switch_query_v2 --new_frame_enable_windows --new_frame_enable_v2_windows --enable_camera_status_fix_1017 --enable_camera_open_limited --conf_concurrency_limit --enable_cell_controller_0515 --enable_conf_upgrade --enable_share_optimize_v2_windows --conf_count_down_limit_v2 --enable_conf_day_meeting --conf_toast_fix --main-orgid=439446171 --selected-org-name=钉钉 --enable_schema_open_join_conf_view --app-environment=0 --self-uid=119732102 --enable-crash --enable-wer';
    //params = cmdLine.split(' ');
    //params.push('--type=conference');
    //NOTE：应用启动参数在此处添加
    this.launchParams = params.join(' ');
  }

  onWindowStageDestroy() {
    // Main window is destroyed, release UI related resources
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onWindowStageDestroy');
  }

  onForeground() {
    // Ability has brought to foreground
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onForeground');
  }

  onBackground() {
    // Ability has back to background
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onBackground');
  }

  /**
   * note：启动模式为singleton/specified才生效
   * @param want
   * @param launchParam
   */
  onNewWant(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    qpa.handleOnNewWant(want, launchParam);
  }

  /**
   * note：该接口需要配置PREPARE_APP_TERMINATE权限（module.json5）
   * @returns
   */
  onPrepareToTerminate() {
    //关闭窗体时通知qt 后面添加
    if (this.closeAbility != undefined) {
      this.closeAbility()
      return true;
    }
    //阻止ts程序退出，由qt程序确定是否退出
    return false;
  }
};

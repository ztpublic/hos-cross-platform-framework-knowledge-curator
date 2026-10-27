> 模板版本: v0.0.2

<p align="center">
  <h1 align="center"> <code>record</code></h1>
</p>

本项目基于 [record](https://pub.dev/packages/record) 开发。

## 1. 安装与使用

### 1.1 安装方式

进入到工程目录并在 pubspec.yaml 中添加以下依赖：

<!-- tabs:start -->

#### pubspec.yaml

```yaml
...

dependencies:
  record:
    git:
      url: https://gitcode.com/CPF-Flutter/fluttertpc_record.git
      path: record
      
...
```

执行命令

```bash
flutter pub get
```

<!-- tabs:end -->

### 1.2 使用案例

使用案例详见 [record/example](/record/example/lib/main.dart)。

## 2. 约束与限制

### 2.1 兼容性

在以下版本中已测试通过

1. Flutter: 3.27.5-ohos-0.0.1; SDK: 5.0.0(12); IDE: DevEco Studio: 5.1.0.828; ROM: 5.1.0.130 SP8;
2. Flutter: 3.35.7-ohos-0.0.1; SDK: 6.0.1(21); IDE: DevEco Studio: 6.0.1.260; ROM: 6.0.0.120 SP6;

## 3. API

> [!TIP] "ohos Support"列为 yes 表示 ohos 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

| Name                | Description                                                  | Type     | Input                                       | Output                    | ohos Support |
| ------------------- | ------------------------------------------------------------ | -------- | ------------------------------------------- | ------------------------- | ------------ |
| start               | Starts new recording session                                 | function | RecordConfig config, {required String path} | Future<void>              | yes          |
| startStream         | Same as [start] with output stream instead of a path         | function | RecordConfig config                         | Future<Stream<Uint8List>> | yes          |
| stop                | Stops recording session and release internal recorder resource | function | /                                           | Future<String?>           | yes          |
| pause               | Pauses recording session                                     | function | /                                           | Future<void>              | yes          |
| resume              | Resumes recording session after [pause]                      | function | /                                           | Future<void>              | yes          |
| isRecording         | Checks if there's valid recording session                    | function | /                                           | Future<void>              | yes          |
| isPaused            | Checks if recording session is paused                        | function | /                                           | Future<void>              | yes          |
| hasPermission       | Checks and requests for audio record permission              | function | /                                           | Future<void>              | yes          |
| listInputDevices    | Lists capture/input devices available on the platform        | function | /                                           | Future<List<InputDevice>> | yes          |
| getAmplitude        | Gets current average & max amplitudes (dBFS)                 | function | /                                           | Future<Amplitude>         | partially    |
| isEncoderSupported  | Checks if the given encoder is supported on the current platform | function | AudioEncoder encoder                        | Future<bool>              | yes          |
| dispose             | Dispose the recorder                                         | function | /                                           | Future<void>              | yes          |
| onStateChanged      | Listen to recorder states [RecordState]                      | function | /                                           | Stream<RecordState>       | yes          |
| onAmplitudeChanged  | Request for amplitude at given [interval]                    | function | Duration interval                           | Stream<Amplitude>         | yes          |
| convertBytesToInt16 | Utility method to get PCM data as signed 16 bits integers    | function | Uint8List bytes                             | List<int>                 | yes          |


## 4. 属性

> [!TIP] "ohos Support"列为 yes 表示 ohos 平台支持该属性；no 则表示不支持；partially 表示部分支持。使用方法跨平台一致，效果对标 iOS 或 Android 的效果。

### RecordConfig

| Name          | Description                                                                   | Type         | ohos Support |
|---------------|-------------------------------------------------------------------------------|--------------|--------------|
| encoder       | The audio encoder to be used for recording                                    | AudioEncoder | yes          |
| bitRate       | The audio encoding bit rate in bits per second                                | int          | yes          |
| sampleRate    | The sample rate for audio in samples per second                               | int          | yes          |
| numChannels   | The numbers of channels for the recording                                     | int          | yes          |
| device        | The device to be used for recording. If null, default device will be selected | InputDevice? | no           |
| autoGain      | The recorder will try to auto adjust recording volume in a limited range      | bool         | no           |
| echoCancel    | The recorder will try to reduce echo                                          | bool         | no           |
| noiseSuppress | The recorder will try to negates the input noise                              | bool         | no           |


### AudioEncoder

| Name       | Description                                         | Type  | ohos Support |
|------------|-----------------------------------------------------|-------|--------------|
| aacLc      | MPEG-4 AAC Low complexity                           | enum  | yes          |
| aacEld     | MPEG-4 AAC Enhanced Low Delay                       | enum  | yes          |
| aacHe      | MPEG-4 High Efficiency AAC (Version 2 if available) | enum  | yes          |
| amrNb      | The AMR (Adaptive Multi-Rate) narrow band speech    | enum  | no           |
| amrWb      | The AMR (Adaptive Multi-Rate) wide band speech      | enum  | no           |
| opus       | Will output to MPEG_4 format container              | enum  | no           |
| flac       | Free Lossless Audio Codec                           | enum  | no           |
| wav        | Waveform Audio (pcm16bit with headers)              | enum  | no           |
| pcm16bits  | Linear PCM 16 bit per sample                        | enum  | no           |


## 5. 遗留问题

1.getAmplitude无法返回录制时最大分贝openharmony无该api
2.当前鸿蒙端AVRecorder无法配合AudioInterruptionMode完成音频中断处理

## 6. 其他


## 7. 开源协议


本项目基于 [BSD-3-Clause](record/LICENSE) ，请自由地享受和参与开源。

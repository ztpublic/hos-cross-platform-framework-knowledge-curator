# QT嵌入Flutter方案
提供一套QT嵌入Flutter页面的方案，以下将该方案实现的桥接层称为embedded。
embedded提供将Flutter页面嵌入到QT中，和Flutter与QT的通信能力。

## 环境依赖
1. 获取[Qt for Openharmony](https://gitee.com/openharmony-sig/qt/wikis) sdk；
2. 获取[Flutter for Openharmony](https://gitcode.com/openharmony-sig/flutter_engine)的flutter.har;




## 构建步骤
1. DevEco Studio创建一个Native C++工程，在module的build-profile.json5配置Qt的sdk依赖
```JSON
  "buildOption": {
    "externalNativeOptions": {
      "path": "./src/main/cpp/CMakeLists.txt",
      "arguments": "-DQT_PREFIX=/path/to/qt/sdk", //Qt sdk路径
      "cppFlags": "",
      "abiFilters": ["arm64-v8a"]
    }
  }
```
将sdk路径\plugins\platforms\libplugins_platforms_qopenharmony.so拷贝至工程module下的libs\arm64-v8a中，将openharmony\qtbase下的文件拷贝覆盖工程module的ETS文件

2. 添加flutter.har, 桥接层embedded.har文件至qt应用工程中，并在qt应用的module配置oh-package.json5依赖，获取embedded.har参考[构建文档](./flutter_embed_into_qt/README.md)
 ```JSON
   "dependencies": {
    "libqtmodule.so": "file:./src/main/cpp/types/libqtmodule", //qt 模块
    "@ohos/flutter_ohos": "file:../har/flutter.har", //flutter 模块
    "embedded": "file:../har/embedded.har" //embedded模块
  }
 ```
在工程级oh-package.json5配置overrides参数flutter.har,否则embedded.har会提示依赖报错
 ```JSON
  "overrides": {
    "@ohos/flutter_ohos": "file:./har/flutter.har" //声明embedded和qt都引用flutter.har
  }
 ```
qt应用模块C++代码可以跨har引用embedded模块的接口。跨Har文件引用使用请参考[官方文档](https://developer.huawei.com/consumer/cn/doc/best-practices-V5/bpta-cross-module-reference-V5)

在cpp文件添加qt应用的main函数入口，并在CMakeLists.txt添加embedded模块依赖，使qt应用能调用embedded模块接口
```cmake
#PACKAGE_FIND_FILE必须要加上，否则qt C++无法跨模块引用embedded的接口
if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

target_link_libraries(qtmodule PRIVATE
    embedded::embeddedbridge #链接时指定embeddedbridge依赖库
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets)
```
3. 在qt应用module的src/main/ets/entryability/EntryAbility.ets中添加如下代码
```TS
onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    FlutterManager.getInstance().pushUIAbility(this);
    EmbeddedBridgeNAPI.getInstance();//需要在应用启动时导出embeddedbridge NAPI
    //...
}

onWindowStageCreate(windowStage: Window.WindowStage) {
    FlutterManager.getInstance().pushWindowStage(this, windowStage);
    //...
}
```
4. 在qt应用module的src/main/ets/pages/Index.ets添加如下代码
```TS
aboutToAppear(): void {
    AppStorage.setOrCreate<UIContext>("context", this.getUIContext());
    EBTransferManager.getInstance().registerMessageTransfer();
    EmbeddedBridgeNAPI.getInstance().registerXComponent({ createMixedXComponent: createMixedXComponent })
}
```
5. 拷贝Flutter应用的编译产物到qt应用工程

Flutter应用产物文件debug与release模式不同，需要区分配置

debug模式：flutter build hap --debug将Flutter应用编译生成的flutterApplication/build/ohos/flutter_assets拷贝到qt应用module的src/main/resources/rawfile下

release模式：flutter build hap --release debug将Flutter应用编译生成的flutterApplication/build/ohos/arm64-v8a/app.so拷贝到qt应用module的libs/arm64-v8a下。

6. 配置声明qt应用使用sdk最新的动态库依赖覆盖embedded.har内打包的qt依赖库。在qt应用module的build-profile.json5中添加如下配置。
```JSON
"buildOption": {
  //.....其他配置
  "nativeLib": {
      "filter": {
        "select": [
          {
            "package": "embedded",
            "include": [
              "libQt5Core.so",
              "libQt5Gui.so",
              "libQt5Widgets.so"
            ]
          }
        ]
      }
    }
}
```

完整工程请参考[示例demo](./example//qt-embed-demo/README.md)

## 开发说明
1. qt创建和销毁Flutter页面
```c++
/* 创建一个qt原生组件作为嵌入 flutter页面的父节点*/
QWidget window;
/* 创建一个布局用于显示 flutter页面*/
QHBoxLayout *pageLayout = new QHBoxLayout();
/* 设置布局*/
window.setLayout(pageLayout);

/* 事件通行初始化的绑定*/ 

QObject::connect(EmbeddedBridgeJsSide::instance(), &EmbeddedBridgeJsSide::sigSendData,
                     EmbeddedBridgeQtSide::instance(), &EmbeddedBridgeQtSide::slotReceiveData);
QObject::connect(EmbeddedBridgeQtSide::instance(), &EmbeddedBridgeQtSide::sigSendData,
                     EmbeddedBridgeJsSide::instance(), &EmbeddedBridgeJsSide::slotReceiveData);

/* 创建并使用页面 ，注册事件回调*/
std::shared_ptr<FlutterEmbeddedQWidget> flutterPage = FlutterEmbeddedQWidget::Create(&window);; 
pageLayout->addWidget(flutterPage->GetInnerWidget());
EmbeddedBridgeQtSide::instance()->registerMessageHandler(leftPageWidget->GetEventHandler());

/*业务逻辑*/
....

/*使用结束记得销毁flutterpage ， 这样安全*/

pageLayout->removeWidget(flutterPage->GetInnerWidget());
FlutterEmbeddedQWidget::Destroy(flutterPage);
```

2. qt传递消息给flutter，信息传递方向QT->ETS->Flutter
embedded在C++侧定义了消息接收和发送的接口，如下：
```C++
example\qt-embed-demo\embedded\src\main\cpp\include\IMessageHandler.h
class IMessageHandler {
public:
    //receive ArkTS message
    virtual void onMessage(const QString &fun, const QString &data) = 0;
    QString getName() const { return handlerName; }
    //send message to ArkTS
    void invokeMethod(const QString &handler, const QString &func, const QString &data);
    
protected:
    explicit IMessageHandler(const QString &name) : handlerName(name) {}
    
private:
    QString handlerName;
};
```
qt应用侧需要实现IMessageHandler，并注册到EmbeddedBridgeQtSide，即可接收和发送消息到ETS，ETS再将消息通过Flutter channel将消息转发到dart
使用参考example\qt-embed-demo\embedded\src\main\cpp\include\FlutterEmbeddedQWidget.h

ArkTS侧还需要实现IMessageTransfer，作为ArkTS的消息接收和发送方，使用参考example\qt-embed-demo\embedded\src\main\ets\flutter\EmbeddedMessageTransfer.ets
```TS
example\qt-embed-demo\embedded\src\main\ets\flutter\IMessageTransfer.ets
export default abstract class IMessageTransfer {
  protected name:string;
  constructor(transferName: string) {
    this.name = transferName
  }
  //QT消息回调
  abstract onQtMessage(fun: string, data: string): ESObject;

  //发送消息给自身id相同QT message handler
  transferMessageToQt(fun: string, data: string): void {
    EmbeddedBridgeNAPI.getInstance().dispatchMessage(this.name, fun, data);
  }

  //发送消息给指定id的QT message handler
  transferMessageToQtHandler(handler: string, fun: string, data: string): void {
    EmbeddedBridgeNAPI.getInstance().dispatchMessage(handler, fun, data);
  }

  getName(): string {
    return this.name;
  }
}
```

3. flutter传递消息给qt，信息传递方向Dart->ETS->QT，Dart与ETS通过channel通信，ETS与QT通过NAPI和信号槽通信，通信识别依赖指定的id。通信参考[flutter demo](./template/flutter/qt_embedded_flutter_page/README.md)

## 常见问题
1. 构建错误“Duplicated files found in module xxx. This may cause unexpected errors at runtime”

embedded.har中包含了qt的依赖库，与当前的qt应用产生同名库版本冲突，需要在qt应用module的build-profile.json5中添加如下配置。
```JSON
"buildOption": {
  //.....其他配置
  "nativeLib": {
      "filter": {
        "select": [
          {
            "package": "embedded",
            "include": [
              //错误提示的动态库名添加到这里
            ]
          }
        ]
      }
    }
}
```
 
import {
  AnyJSBundleProvider,
  MetroJSBundleProvider,
  RNApp,
  RNOHErrorDialog,
  ResourceJSBundleProvider,
  RNOHCoreContext
} from '@rnoh/react-native-openharmony';
import { getRNOHPackages } from '../PackageProvider';

@Entry
@Component
struct Index {
  @StorageLink('RNOHCoreContext') private rnohCoreContext: RNOHCoreContext | undefined = undefined

  build() {
    Column() {
      if (this.rnohCoreContext) {
        if (this.rnohCoreContext?.isDebugModeEnabled) {
          RNOHErrorDialog({ ctx: this.rnohCoreContext })
        }
        RNApp({
          rnInstanceConfig: {
            name: "AwesomeProject",
            createRNPackages: getRNOHPackages,
            fontResourceByFontFamily: {},
            enableDebugger: this.rnohCoreContext?.isDebugModeEnabled,
          },
          appKey: "AwesomeProject",
          /**
           * 按顺序尝试加载 bundle：
           * 1. Metro 服务（npm start，支持热加载）
           * 2. hermes_bundle.hbc（Hermes 字节码，生产环境推荐）
           * 3. bundle.harmony.js（npm run dev 生成的本地 bundle）
           */
          jsBundleProvider: new AnyJSBundleProvider([
            new MetroJSBundleProvider(),
            new ResourceJSBundleProvider(this.rnohCoreContext.uiAbilityContext.resourceManager, 'hermes_bundle.hbc'),
            new ResourceJSBundleProvider(this.rnohCoreContext.uiAbilityContext.resourceManager, 'bundle.harmony.js'),
          ]),
        })
      }
    }
    .height('100%')
    .width('100%')
  }
}

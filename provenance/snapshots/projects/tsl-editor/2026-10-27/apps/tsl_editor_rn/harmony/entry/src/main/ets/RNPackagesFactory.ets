import type { RNPackageContext, RNPackage } from '@rnoh/react-native-openharmony';
import { NativeUtilsPackage } from './NativeUtilsPackage';
import { HarmonySqlitePackage } from './HarmonySqlitePackage';
import { HarmonyExportPackage } from './HarmonyExportPackage';
// lucide 图标底层：react-native-svg 的鸿蒙 CAPI 包。
// 缺它时所有 Svg/Path/Circle 组件在端上无 ComponentJSIBinder，图标渲染为空白。
// 注意：SvgPackage extends RNPackage（未迁移到 RNOHPackage），
// 而 RNInstance.createRNPackages 的官方契约正是 (ctx) => RNPackage[]，
// 运行时用 instanceof 区分是否 RNOHPackage，所以这里用 RNPackage[] 而不是 RNOHPackage[]。
import { SvgPackage } from '@react-native-ohos/react-native-svg/ts';

export function createRNPackages(ctx: RNPackageContext): RNPackage[] {
  return [
    new NativeUtilsPackage(ctx),
    new HarmonySqlitePackage(ctx),
    new HarmonyExportPackage(ctx),
    new SvgPackage(ctx),
  ];
}

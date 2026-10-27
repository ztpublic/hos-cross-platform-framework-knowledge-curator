import { UITurboModule } from '@rnoh/react-native-openharmony/ts';
import deviceInfo from '@ohos.deviceInfo';
import process from '@ohos.process';
import hidebug from '@ohos.hidebug';

// Expo's enum has no 2-in-1 value; expose it as DESKTOP, matching the closest
// consumer-facing category instead of leaking the OpenHarmony-only value.
const typeMap: Record<string, number> = {unknown: 0, phone: 1, tablet: 2, '2in1': 3, tv: 4};
const nullable = (value: string | undefined): string | null => value && value.length > 0 ? value : null;

export class ExpoDeviceTurboModule extends UITurboModule {
  getConstants() {
    const memory = Number(hidebug.getSystemMemInfo().totalMem) * 1024;
    return {
      isDevice: true,
      brand: nullable(deviceInfo.brand),
      manufacturer: nullable(deviceInfo.manufacture),
      modelId: null,
      modelName: nullable(deviceInfo.marketName) ?? nullable(deviceInfo.productModel),
      designName: nullable(deviceInfo.productModel),
      productName: nullable(deviceInfo.productSeries),
      deviceType: typeMap[deviceInfo.deviceType] ?? 0,
      deviceYearClass: null,
      totalMemory: Number(memory),
      supportedCpuArchitectures: nullable(deviceInfo.abiList)?.split(',').map(value => value.trim()).filter(Boolean) ?? null,
      osName: 'OpenHarmony',
      osVersion: nullable(deviceInfo.osFullName),
      osBuildId: nullable(deviceInfo.displayVersion),
      osInternalBuildId: nullable(deviceInfo.incrementalVersion),
      osBuildFingerprint: nullable(deviceInfo.versionId),
      platformApiLevel: Number(deviceInfo.sdkApiVersion) || null,
      deviceName: nullable(deviceInfo.marketName) ?? nullable(deviceInfo.productModelAlias),
    };
  }

  async getDeviceTypeAsync(): Promise<number> {
    return typeMap[deviceInfo.deviceType] ?? 0;
  }

  async getUptimeAsync(): Promise<number> {
    return process.uptime() * 1000;
  }

  async getMaxMemoryAsync(): Promise<number> {
    const limit = hidebug.getAppMemoryLimit();
    return Number(limit.vmHeapLimit ?? limit.vmTotalHeapSize) * 1024;
  }

  async isRootedExperimentalAsync(): Promise<boolean> {
    throw new Error('ERR_EXPO_DEVICE_UNAVAILABLE: isRootedExperimentalAsync is not available on OpenHarmony');
  }

  async isSideLoadingEnabledAsync(): Promise<boolean> {
    throw new Error('ERR_EXPO_DEVICE_UNAVAILABLE: isSideLoadingEnabledAsync is not available on OpenHarmony');
  }

  async getPlatformFeaturesAsync(): Promise<string[]> {
    return [];
  }

  async hasPlatformFeatureAsync(_feature: string): Promise<boolean> {
    return false;
  }
}

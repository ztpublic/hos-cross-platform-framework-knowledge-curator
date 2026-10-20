import {
	AnyJSBundleProvider,
	ComponentBuilderContext,
	FileJSBundleProvider,
	MetroJSBundleProvider,
	ResourceJSBundleProvider,
	RNApp,
	RNOHCoreContext,
	RNOHErrorDialog,
	TraceJSBundleProviderDecorator,
} from '@rnoh/react-native-openharmony';
import { RNPackageContext } from '@rnoh/react-native-openharmony/ts';
import { ClipboardPackage } from '@react-native-ohos/clipboard';
import { NetInfoPackage } from '@react-native-ohos/netinfo';
import { RNDeviceInfoPackage } from '@react-native-ohos/react-native-device-info';
import { DocumentPickerPackage } from '@react-native-ohos/react-native-document-picker/ts';
import { RNFileViewerPackage } from '@react-native-ohos/react-native-file-viewer/ts';
import { ImagePickerViewPackage } from '@react-native-ohos/react-native-image-picker';
import { RNInAppBrowserPackage } from '@react-native-ohos/react-native-inappbrowser-reborn/ts';
import { RNLocalizePackage } from '@react-native-ohos/react-native-localize';
import { PermissionsPackage } from '@react-native-ohos/react-native-permissions/ts';
import { SafeAreaViewPackage } from '@react-native-ohos/react-native-safe-area-context';
import { RNSharePackage } from '@react-native-ohos/react-native-share';
import { SQLitePluginPackage } from '@react-native-ohos/react-native-sqlite-storage/ts';
import { SvgPackage } from '@react-native-ohos/react-native-svg';
import { WebViewPackage } from '@react-native-ohos/react-native-webview/ts';
import { ZipArchivePackage } from '@react-native-ohos/react-native-zip-archive';
import JoplinHarmonyPackage from '../rn/JoplinHarmonyPackage';
import { JoplinHarmonyTextPromptHost } from '../rn/JoplinHarmonyTextPromptHost';

@Builder
export function buildCustomRNComponent(ctx: ComponentBuilderContext) {}

const wrappedCustomRNComponentBuilder = wrapBuilder(buildCustomRNComponent);

@Entry
@Component
struct Index {
	@StorageLink('RNOHCoreContext') private rnohCoreContext: RNOHCoreContext | undefined = undefined;
	@State shouldShow: boolean = false;

	aboutToAppear(): void {
		this.shouldShow = true;
	}

	onBackPress(): boolean | undefined {
		this.rnohCoreContext!.dispatchBackPress();
		return true;
	}

	build() {
		Stack() {
			if (this.rnohCoreContext && this.shouldShow) {
				if (this.rnohCoreContext.isDebugModeEnabled) {
					RNOHErrorDialog({ ctx: this.rnohCoreContext });
				}
				RNApp({
					rnInstanceConfig: {
						createRNPackages: (ctx: RNPackageContext) => [
							new ClipboardPackage(ctx),
							new NetInfoPackage(ctx),
							new RNDeviceInfoPackage(ctx),
							new DocumentPickerPackage(ctx),
							new RNFileViewerPackage(ctx),
							new ImagePickerViewPackage(ctx),
							new RNInAppBrowserPackage(ctx),
							new RNLocalizePackage(ctx),
							new PermissionsPackage(ctx),
							new SafeAreaViewPackage(ctx),
							new RNSharePackage(ctx),
							new SQLitePluginPackage(ctx),
							new SvgPackage(ctx),
							new WebViewPackage(ctx),
							new ZipArchivePackage(ctx),
							new JoplinHarmonyPackage(ctx, this.rnohCoreContext!.uiAbilityContext),
						],
						enableNDKTextMeasuring: true,
						enableBackgroundExecutor: false,
						enableCAPIArchitecture: true,
						fontResourceByFontFamily: {
							'FontAwesome5Free-Regular': $rawfile('fonts/FontAwesome5_Regular.ttf'),
							'FontAwesome5Free-Solid': $rawfile('fonts/FontAwesome5_Solid.ttf'),
							'FontAwesome5Brands-Regular': $rawfile('fonts/FontAwesome5_Brands.ttf'),
							'Ionicons': $rawfile('fonts/Ionicons.ttf'),
							'MaterialDesignIcons': $rawfile('fonts/MaterialDesignIcons.ttf'),
							'MaterialIcons-Regular': $rawfile('fonts/MaterialIcons.ttf'),
							'material': $rawfile('fonts/MaterialIcons.ttf'),
							'material-community': $rawfile('fonts/MaterialCommunityIcons.ttf'),
						},
						arkTsComponentNames: [],
					},
					appKey: 'Joplin',
					wrappedCustomRNComponentBuilder,
					jsBundleProvider: new TraceJSBundleProviderDecorator(
						new AnyJSBundleProvider([
							new ResourceJSBundleProvider(this.rnohCoreContext.uiAbilityContext.resourceManager, 'hermes_bundle.hbc'),
							new ResourceJSBundleProvider(this.rnohCoreContext.uiAbilityContext.resourceManager, 'bundle.harmony.js'),
							new FileJSBundleProvider('/data/storage/el2/base/files/bundle.harmony.js'),
							new MetroJSBundleProvider('http://localhost:8081/packages/app-mobile/index.bundle?platform=harmony&dev=true&minify=false'),
						]),
						this.rnohCoreContext.logger,
					),
				});
				JoplinHarmonyTextPromptHost();
			}
		}
		.width('100%')
		.height('100%')
	}
}

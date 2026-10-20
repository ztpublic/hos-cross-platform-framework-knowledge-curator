---
{
  "schema_version": 1,
  "id": "rnoh-js-bundle-platform",
  "title": "Serve a harmony JS bundle through Metro",
  "topic_key": "build.rnoh-js-bundle-platform",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "react-native"
  ],
  "summary": "Add harmony to Metro's platform resolution, merge the RNOH Metro configuration, select RNOH's InitializeCore for that platform, and ship an explicitly built harmony bundle.",
  "when_to_use": "The OH app starts but shows a blank screen, cannot resolve a .harmony module, or fails inside InitializeCore while the iOS and Android bundles work.",
  "keywords": [
    "Metro",
    "harmony platform",
    "bundle-harmony",
    "createHarmonyMetroConfig",
    "InitializeCore",
    "JSBundleProvider",
    "Hermes",
    "React Native"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH 2in1/tablet apps built with RNOH. The Metro package, CLI and bundle format must match the React Native and RNOH versions.",
  "related": [
    "rnoh-native-host-and-codegen",
    "rnoh-ohos-package-adaptation"
  ]
}
---

# Serve a harmony JS bundle through Metro

## When to use

The OH app starts but shows a blank screen, cannot resolve a `.harmony` module, or fails inside `InitializeCore` while the iOS and Android bundles work.

## Recommended approach

Add `harmony` to Metro's platform resolution, merge the RNOH Metro configuration, select RNOH's `InitializeCore` for that platform, and ship an explicitly built harmony bundle.

## Implementation steps

1. Add the harmony platform to `resolver.platforms` and merge the RNOH Metro configuration with the default configuration.
2. Override `serializer.getModulesRunBeforeMainModule` so the harmony platform loads RNOH's `InitializeCore` and other platforms keep the default core module. A platform entry without this selection boots the wrong runtime.
3. Preserve the project's asset, alias and styling resolver configuration after merging; add any font or image assets required by the harmony build.
4. Build with the RNOH bundle command for the target mode (development or release) and place the output where the app can read it.
5. In the root view, provide a bundle-provider chain: Metro for development, then a file provider, then a packaged rawfile or Hermes bundle. The first available provider wins.
6. Verify the bundle filename and the provider path refer to the same file, and that the app key still matches the registered component.
7. Re-run the Android and iOS bundling paths to confirm the platform addition did not change them.

## Conditions and alternatives

The RNOH Metro package and CLI must match the React Native version. Hermes bytecode and plain JavaScript bundles need different providers and filenames. A custom platform-capture middleware is only needed for tooling that loses the platform parameter; it is not part of the runtime contract.

## Common mistakes

- Adding `harmony` to the platform list without merging the RNOH Metro configuration.
- Loading the default `InitializeCore` on harmony, producing a runtime that starts but cannot resolve RNOH globals.
- Shipping the iOS or Android bundle to the OH app.
- Adding a bundled-asset path that is not packaged into `rawfile`.
- Testing only the development server and never the release bundle.

## Check it works

Launch the OH app with the development server, then with the packaged release bundle. Both should render the same root component. Confirm a `.harmony`-specific module resolves, no `InitializeCore` error appears, and the Android/iOS bundles still build.

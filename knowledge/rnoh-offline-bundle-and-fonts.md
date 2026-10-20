---
{
  "schema_version": 1,
  "id": "rnoh-offline-bundle-and-fonts",
  "title": "Ship an RNOH app with an offline bundle, assets and fonts",
  "topic_key": "build.rnoh-offline-bundle-and-fonts",
  "area": "build",
  "scope": "framework-specific",
  "frameworks": [
    "react-native"
  ],
  "summary": "Chain Hermes, bundled JS, sandbox file and Metro bundle providers; generate an offline harmony bundle with assets/fonts; and map icon font families to rawfile resources.",
  "when_to_use": "An RNOH app only works with Metro running, shows a blank screen offline, or loses icons and images because its fonts/assets are not packaged in the HAP.",
  "keywords": [
    "RNOH",
    "bundle.harmony.js",
    "hermes_bundle.hbc",
    "JSBundleProvider",
    "Metro",
    "rawfile",
    "fonts",
    "assets",
    "RNApp",
    "offline bundle"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run",
  "applicability": "OH React Native/RNOH apps using the RNOH bundle-provider API. Bundle format, font mapping and provider order are RNOH-version specific.",
  "related": [
    "rnoh-js-bundle-platform",
    "rnoh-native-host-and-codegen",
    "rnoh-har-reactcommon-extraction"
  ]
}
---

# Ship an RNOH app with an offline bundle, assets and fonts

## When to use

An RNOH app only works with Metro running, shows a blank screen offline, or loses icons and images because its fonts/assets are not packaged in the HAP.

## Recommended approach

Chain Hermes, bundled JS, sandbox file and Metro bundle providers; generate an offline harmony bundle with its assets and fonts; and map icon font families to rawfile resources.

## Implementation steps

1. Build the TypeScript/JavaScript entry for the harmony platform and any injected WebView scripts before generating the bundle.
2. Run the offline bundle task so it writes `bundle.harmony.js`, copies React Native assets to `rawfile/assets/`, and copies icon fonts to `rawfile/fonts/`.
3. In `RNApp`, build an `AnyJSBundleProvider` chain: packaged Hermes bundle, packaged `bundle.harmony.js`, a sandbox file path, then the Metro URL for development. Wrap it with the trace decorator if one is provided.
4. Set the app key to the name passed to `AppRegistry.registerComponent`, and register every OH package the JS side imports in `createRNPackages`.
5. Map each icon font family to `$rawfile('fonts/<file>.ttf')` in `fontResourceByFontFamily`; a missing mapping renders blank glyphs even when the font file is present.
6. In development, keep Metro running and use a reachable host address; `localhost` may not resolve from the device/emulator to the development machine.
7. Verify an offline install (no Metro), then a Metro dev session, and ensure the packaged bundle is not stale after JS changes.

## Conditions and alternatives

Hermes bytecode and plain JS bundles need different providers and filenames. The provider order determines which bundle wins; putting Metro first makes the app depend on the development server. RNOH versions may rename provider classes.

## Common mistakes

- Registering only the Metro provider and then testing offline.
- Forgetting to copy assets or fonts into rawfile.
- Mapping a font family to the wrong filename or not mapping it at all.
- Using an app key that differs from the registered component.
- Shipping a stale bundled JS file after changing the TypeScript source.

## Check it works

Install without Metro and confirm the app opens, images render and icon glyphs are correct. Then start Metro and confirm the development bundle is used. Change a visible string, regenerate the offline bundle, rebuild and confirm the new string appears without Metro.

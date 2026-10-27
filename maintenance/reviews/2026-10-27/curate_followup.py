"""Persist follow-up discoveries; reuse only the definitions from our local recipe."""
from pathlib import Path
import json

HERE=Path(__file__).resolve().parent
# Avoid replaying earlier mutations when sharing the snapshot/authoring helpers.
exec(compile((HERE/'curate.py').read_text().split("K='core-render-ohos/")[0],str(HERE/'curate.py'),'exec'))
LICENSES.update({'tauri-open-harmony':'MIT OR Apache-2.0; existing notices retained.','harmony-tauri-app':'No repository-level license found; derivative upstream components retain their notices.','bitfun-ohos':'MIT root notice; dependency and vendored component terms apply.','tsl-editor':'MIT','rnoh-on-harmony-pc':'No standalone license notice found; README attributes RNOH-related code to Huawei under MIT.'})

add('tauri-ohos-production-custom-protocol','Build embedded Tauri OH assets in production protocol mode','host',['tauri'],
 'Enable the selected Tauri fork\'s custom-protocol feature for packaged assets and check the effective frontend distribution path; release optimization alone does not select production asset loading.',
 'A packaged Tauri OH webview reports Failed to request tauri://localhost or attempts to reach a development server after the HAP is installed offline.',
 'Experimental Tauri OH forks where build.rs derives dev as the inverse of custom-protocol. Apply only after checking the pinned fork and effective config; the source reports a PC run, not curator validation.',
 'custom-protocol|cfg dev|frontendDist|devUrl|tauri://localhost|offline|white screen',
 ['tauri-ohos-ability-white-screen','tauri-ohos-explicit-config-overlay','tauri-ohos-dependency-cohort'],
 ['Inspect the pinned framework build.rs and the application\'s feature graph. In the reviewed generation, `dev = !has_feature("custom-protocol")`.',
  'Enable `tauri/custom-protocol` through a production feature or dependency feature when packaging embedded frontend assets. Do not assume `--release` enables it.',
  'Verify `frontendDist` resolves to the prepared static output and the correct OH config overlay is active. Confirm the custom-scheme request serves packaged assets rather than forwarding to devUrl.',
  'Keep development-server builds intentionally separate and preserve the matching Ability/runtime dependency cohort.'],
 'The source exposes a static global Tauri frontend; this does not imply every bundler or newer CLI needs a manual feature flag. A white screen can also come from Ability mismatch, missing IPC or frontend exceptions.',
 'Changing the URL alone does not create embedded assets. Mixing an old CLI with a new build/runtime can change feature/config semantics.',
 'Install the packaged build with the dev server stopped and network unavailable, load nested assets and invoke a Rust command. Check that no development-host requests occur.',
 [('harmony-tauri-app','src-tauri/Cargo.toml'),('harmony-tauri-app','src-tauri/Cargo.lock'),('harmony-tauri-app','src-tauri/tauri.conf.json'),('harmony-tauri-app','src-tauri/gen/ohos/entry/src/main/ets/entryability/EntryAbility.ets'),('tauri-open-harmony','crates/tauri/build.rs')])

add('native-statusbar-event-routing','Route OH PC status-bar clicks without an extra popup','window',[],
 'Use the status-bar event path for restoring an existing window, while giving notify-only menu actions a resolvable Ability identity and an explicit menu code.',
 'Clicking a PC status-bar icon opens an empty popup, or a notifyOnly right-click action reports no opening method instead of invoking the running app.',
 'Huawei Desktop Extension Kit statusBarManager on products exposing SystemCapability.PCService.StatusBarManager. SDK/API and service authorization must be checked; this is not generic OpenHarmony tray availability.',
 'statusBarManager|quickOperation|statusBarIconClick|rightMenuClick|notifyOnly|menuCode|StatusBarMenuAction',
 ['electron-ohos-statusbar-openurl','qt-tray-effective-capability','privileged-capability-gating'],
 ['Check the status-bar syscap without making an optional PC surface an install requirement for every device. Keep the main window usable when registration fails.',
  'For the reviewed event route, provide the required quickOperation fields including positive height and use an empty quickOperation abilityName; subscribe to statusBarIconClick and restore/focus the existing window.',
  'For right-menu actions, set a real moduleName/abilityName even with notifyOnly=true, plus a unique menuCode. Dispatch rightMenuClick by that code.',
  'Tie registration, event handlers and any menu-refresh timer to the host lifetime. Roll back partial installation and release image resources; record a failed removal rather than assuming the item disappeared.'],
 'The two Ability-name rules apply to different fields. The reviewed service uses API-21-compatible menu labels instead of the selected property introduced later. Source icon cleanup and partial-registration rollback are incomplete, so the whole service is not a drop-in lifecycle implementation.',
 'Using the same empty Ability name for a right-menu action can prevent resolution. Launching a statusBarView extension for every icon click can create a visible intermediate window.',
 'Test left/right clicks, repeat install/remove, missing capability, registration failure, foreground/background shutdown and a recreated main window. Confirm one live target receives each event.',
 [('bitfun-ohos','src/apps/ohos/entry/src/main/ets/services/StatusBarTrayService.ets'),('bitfun-ohos','src/apps/ohos/entry/src/main/ets/entryability/EntryAbility.ets'),('bitfun-ohos','src/apps/ohos/entry/src/main/module.json5'),('bitfun-ohos','src/apps/ohos/oh-rs-ability/oh-package.json5')],limitations=['Selected native service only; no claim of full application feature review, generic tray support or runtime-tested teardown.'])

add('rnoh-relationalstore-sql-contract','Preserve SQL semantics in an RNOH relationalStore adapter','storage',['react-native'],
 'Implement a narrow UITurboModule SQL contract over relationalStore with explicit result types, transactions and schema-version metadata rather than treating it as a transparent SQLite driver.',
 'An RN database abstraction needs an OH backend but its expected SQL/query/version operations do not map directly onto the selected relationalStore surface.',
 'RNOH applications with an app-owned SQL TurboModule using UIAbilityContext and OH RdbStore. PC database persistence and concurrent RN windows require separate tests; SQL support is API/backend specific.',
 'UITurboModule|relationalStore|RdbStore|querySql|executeSql|ResultSet|schema version|transaction',
 ['rnoh-native-host-and-codegen','sqlite-read-write-transaction-contention','batched-import-partial-commit'],
 ['Register the exact JS module name in the ArkTS UITurboModule factory and match the host RNOH package/version; obtain the store from the owning UIAbilityContext.',
  'Define query row/value conversion and execute semantics explicitly. Close every ResultSet in finally, including conversion failures.',
  'If the selected abstraction cannot use its expected version PRAGMA, maintain a dedicated metadata table and update version plus schema changes within the same migration transaction.',
  'Expose begin/commit/rollback as an owned transaction contract. Serialize migration and connection initialization; reject conflicting operations.',
  'Accept structured statements/parameters for batches or use a real SQL parser. Never copy a `split(";")` script executor into a general adapter.'],
 'The source demonstrates registration, query conversion and metadata-table routing. Its PRAGMA comment is a source-specific compatibility claim, not a platform-wide prohibition. Its split-based batches, initialization races and success-only ResultSet cleanup need hardening.',
 'A SQL-looking API does not guarantee SQLite PRAGMA, migration or transaction equivalence. Semicolons in strings/triggers make naive script splitting incorrect.',
 'Test typed null/blob/numeric values, query failure cleanup, simultaneous open calls, migration rollback, semicolons inside values and reopening after process death.',
 [('tsl-editor','apps/tsl_editor_rn/harmony/entry/src/main/ets/HarmonySqliteTurboModule.ets'),('tsl-editor','apps/tsl_editor_rn/harmony/entry/src/main/ets/HarmonySqlitePackage.ets'),('tsl-editor','apps/tsl_editor_rn/harmony/entry/oh-package.json5'),('tsl-editor','apps/tsl_editor_rn/harmony/entry/src/main/ets/RNPackagesFactory.ets')],limitations=['Guidance separates the reusable adapter shape from source defects; no data migration or runtime SQL checks were executed.'])

extend('napi-thread-and-handle-lifetime',[('harmony-tauri-app','src-tauri/src/lib.rs')],
 'Added source examples distinguishing a VM-thread FunctionRef call from worker ThreadsafeFunction dispatch. Process-global callback storage is example scope; teardown/replacement and lock reentrancy remain caller responsibilities.')
extend('rnoh-offline-bundle-and-fonts',[('rnoh-on-harmony-pc','harmony/entry/src/main/ets/pages/Index.ets'),('rnoh-on-harmony-pc','harmony/entry/oh-package.json5'),('rnoh-on-harmony-pc','package.json'),('rnoh-on-harmony-pc','metro.config.js')],
 'Added API-26/RN-0.84 source example selecting a packaged JS fallback when the expected Hermes bytecode resource is absent. Source-reported PC success remains unexecuted; no broad inference of package/backend compatibility.')

old=json.loads((HERE/'curation-manifest.json').read_text())
old['added']+=ADDED
old['extended']=sorted(set(old['extended']+EXTENDED))
old['snapshots']+=list(SNAPS.values())
old['source_ids']=sorted(set(old['source_ids']+[k[0] for k in SNAPS]))
dump(HERE/'curation-manifest.json',old)
print(json.dumps(dict(added=ADDED,extended=EXTENDED,total_added=len(old['added']),snapshots=len(old['snapshots'])),indent=2))

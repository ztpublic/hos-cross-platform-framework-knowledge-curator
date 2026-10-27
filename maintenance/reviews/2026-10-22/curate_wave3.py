import json
import pathlib
import runpy
HERE=pathlib.Path(__file__).resolve().parent
g=runpy.run_path(str(HERE/'curate_wave2.py'))
# Functions in the first script retain their shared evidence dictionaries.
atom=g['atom']; snapshot=g['snapshot']; dump=g['dump'];DB=g['DB'];first=atom.__globals__
info=json.loads((HERE/'vscodium-source.json').read_text())
g['sources']['vscodium-ohos']=dict(bare='hos_vscodium',url=info['url'],revision=info['revision'],ref=info['ref'],framework='electron',license='No explicit license established for the custom native bridge sources; bundled VSCodium/Electron and dependencies retain their upstream licenses.')

atom('electron-ohos-napi-symbol-owner','Resolve addon NAPI calls against the owning Electron VM','native','electron',
 'An OH Electron native addon has the correct Node ABI but fails to resolve NAPI symbols or crashes after binding to the ArkTS VM instead of Electron\'s environment.',
 'For the affected runtime, resolve the required NAPI entrypoints from the actual loaded Electron library and forward addon calls consistently to that owner.',
 'Electron OH fork loading libelectron.so with local symbol visibility and exposing multiple NAPI providers. PC behavior depends on the exact runtime, SDK, addon ABI and authorized loader policy.',
 ['RTLD_LOCAL','RTLD_NOLOAD','dlsym','libelectron.so','napi-dyn','NAPI','ArkTS VM','native addon'],
 ['electron-ohos-native-addon-abi','ffi-library-and-symbol-availability','native-event-loop-handle-teardown'],
 ['Confirm architecture and Node ABI first, then inspect how the host loads Electron and which library supplies each NAPI symbol. Matching symbol names do not establish VM compatibility.',
  'Retain a handle to the intended already-loaded libelectron.so, using RTLD_NOLOAD where supported. Resolve the explicitly required NAPI symbol set against that handle, not the process-wide default provider.',
  'Route all addon NAPI calls through the resolved function table and keep direct undefined NAPI imports out of the addon when this runtime requires that strategy. Check every required resolution before module registration.',
  'Keep node headers, signatures and function-pointer declarations compatible with the target ABI. Initialize before other addon constructors can call NAPI, and keep the provider library alive while the addon runs.',
  'Handle missing runtime/symbols as a clear initialization failure. Do not silently load an unrelated second runtime or mix a napi_env from one VM with functions from another.',
  'Test any runtime-specific addon preload requirement before renderer startup separately; an early require cache can mask a later environment-lifetime defect.'],
 'This is a scoped adapter for local symbol visibility and multiple NAPI providers, not a general replacement for normal Node addon linkage. The reviewed implementation resolves in a constructor and aborts on missing symbols; a consumer must choose its intended failure policy and validate provider identity.',
 'Do not bind to the first same-named NAPI export, assume linking another libelectron guarantees the correct environment, or infer symbol ownership from ABI equality.',
 'Load the addon before and after renderer creation in the intended signed PC app. Inspect resolved provider addresses and exercise representative callbacks and teardown; reject a missing/mismatched provider without executing against another VM.',
 [('vscodium-ohos','native/src/napi-dyn.c'),('vscodium-ohos','native/src/napi-dyn.h'),('vscodium-ohos','native/build.sh'),('vscodium-ohos','README.md')],
 ['No loader/VM behavior was reproduced. Source runtime-specific preload and device claims remain reports.'])

atom('electron-ohos-conditional-pty-adapter','Implement a PTY adapter only where the app can allocate one','integration','electron',
 'A packaged OH Electron terminal needs node-pty behavior, but the desktop addon crashes when ptsname returns null or the selected app context denies PTY allocation.',
 'Probe PTY capability in the actual app context, then implement only the terminal API subset supported by that runtime; retain a non-PTY or remote terminal fallback when allocation is denied.',
 'OH Electron runtime and signed application identity with genuinely working forkpty/exec and permitted shell artifacts. Source device claims include different app identities; ordinary-app PTY availability is not established here.',
 ['node-pty','forkpty','ptsname','TIOCSWINSZ','waitpid','pthread','napi_threadsafe_function','terminal','executableBinaryPaths'],
 ['electron-ohos-non-pty-exec','electron-ohos-napi-symbol-owner','native-dependency-closure'],
 ['Exercise forkpty and shell startup inside the intended app using its own bundle identity and legitimate signing/profile. SDK exports or hdc shell success do not establish application sandbox permission.',
  'If allocation is denied, keep the feature unavailable or use the established non-PTY/remote path. Do not adopt another application\'s privileged identity as a portability technique.',
  'Where allocation works, audit the consumer\'s actual addon API subset. The reviewed custom adapter implements fork, resize and process lookup without relying on ptsname(master) for the terminal name.',
  'Launch only the intended permitted shell with a controlled argument/environment/working-directory contract. Package/register executable artifacts as required by the selected product SDK; this is separate from PTY authorization.',
  'Implement resize with the terminal size ioctl. Reap the child with waitpid, decode exit/signal status, and deliver completion to the correct JS thread through a retained TSFN.',
  'Define failure and teardown paths for fork, exec, watcher creation, TSFN delivery, master descriptor close and engine destruction. A detached wait thread must not retain an invalid environment.'],
 'This is a conditional PTY integration. The source reports device terminal success but also describes a system-app identity variant; the available report does not isolate which privilege/profile enables each behavior. Existing non-PTY guidance remains valid where allocation or child-exit delivery fails.',
 'Do not equate compiled POSIX symbols with runtime permissions, dereference a null ptsname result, assume the full node-pty API is supported, or claim a non-PTY batch shell provides interactive terminal semantics.',
 'On the target PC and intended app identity verify input/echo, resize, Unicode, EOF, shell exit/status, signals, route closure and engine teardown. Record product/API/profile and repeat with the unprivileged supported configuration before making portability claims.',
 [('vscodium-ohos','native/src/ohpty.c'),('vscodium-ohos','electron/src/main/module.json5'),('vscodium-ohos','README.md')],
 ['No terminal or privileged identity was executed during curation. In-source TSFN/watcher failure paths still require audit; no claim of full node-pty equivalence.'])

first['extend']('electron-ohos-non-pty-exec',
 'Non-PTY `fork` + `execve` + pipes work, but there is no TTY, no job control and no reliable process-exit event.',
 'In the runtime that motivated this adapter, non-PTY `fork` + `execve` + pipes work, but there is no TTY, no job control and no reliable process-exit event. This is not a product-wide prohibition: a separate runtime has a source-backed [conditional PTY adapter](electron-ohos-conditional-pty-adapter.md), with app-identity and permission limits. Validate the intended signed app context before choosing either route.',
 [('vscodium-ohos','native/src/ohpty.c'),('vscodium-ohos','README.md')],['electron-ohos-conditional-pty-adapter'])
manifest=json.loads((HERE/'curation-manifest.json').read_text())
manifest.update(added_entries=first['atoms'],extended_entries=first['extended'],sources=g['sources'],snapshots=list(first['records'].values()))
dump(HERE/'curation-manifest.json',manifest)
print(json.dumps(dict(added_total=len(first['atoms']),extended=len(first['extended']),snapshots=len(first['records'])),indent=2))

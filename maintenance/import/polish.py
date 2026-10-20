"""Reviewed refinements applied to the generated import before persistence."""
EXTRA_REPLACEMENTS={
 'its Dart dependencies':'its runtime dependencies','a late Dart round trip':'a late application round trip',
 'telling Dart':'telling the application','a stale Dart flag':'a stale application flag',
 'A Flutter OH app':'An OH application','Trace Dart path/Git dependencies':'Trace package path/Git dependencies',
 'A Dart wrapper':'A language wrapper','Flutter publishing':'the application runtime publishing',
 'backed by Flutter data':'backed by application data','until Flutter handlers':'until application handlers',
 'Disabling a Flutter home-widget package':'Disabling one framework home-widget package',
 'A Flutter scanner':'A cross-platform scanner','Put Dart cleanup':'Put caller cleanup',
 '`startScan` plus `EventChannel`':'a start operation plus an event stream',
 'Pure Dart socket/HTTP/SSH logic':'Portable socket/HTTP/SSH logic','delayed Dart readiness':'delayed runtime readiness',
 'or the Dart API':'or the shared API','The Dart filename/path':'The shared-layer filename/path',
 'a standard OH-capable Flutter WebView':'a standard OH-capable WebView adapter',
 'not a claim that OH lacks Flutter visual effects':'not a claim that the OS lacks those visual effects',
 'before merging with Flutter `padding`/`viewPadding`':'before merging with the shared layout\'s existing system insets',
 'Dart application':'shared application','Dart Future':'application future',
}
for ident,(meta,body) in list(ENTRIES.items()):
    if meta['scope']=='common':
        for old,new in EXTRA_REPLACEMENTS.items():
            body=body.replace(old,new)
            for key in ['title','summary','when_to_use','applicability']:meta[key]=meta[key].replace(old,new)
        if not re.search(r'\bFlutter\b|\bDart\b|MethodChannel|EventChannel|BinaryCodec',body.replace("Named Flutter/Dart APIs below are adapter examples. The shared contract applies only through an implementation with equivalent native semantics; it does not establish other frameworks' support.",'')):
            body=body.replace("\nNamed Flutter/Dart APIs below are adapter examples. The shared contract applies only through an implementation with equivalent native semantics; it does not establish other frameworks' support.\n",'')
    # Synchronize visible title and indexed symptom after prose generalization.
    body=re.sub(r'^# .+\n','# '+meta['title']+'\n',body,count=1)
    ENTRIES[ident]=(meta,body)

# Split paragraphs which had accumulated inside otherwise independent subsections.
def move_paragraph(parent,text,newid,title,area,scope,when,summary,conditions,mistakes,checks):
    meta,body=ENTRIES[parent]
    if text not in body:raise ValueError('Missing paragraph in '+parent)
    sources=[(x['collection'],x['original_path']) for x in PROVS[parent]['sources']]
    add(newid,title,area,scope,summary,when,text,conditions,mistakes,checks,sources,meta['keywords'],[parent])
    body=body.replace(text,f'See [{title}]({newid}.md).')
    meta['related'].append(newid);ENTRIES[parent]=(meta,body)
    PROVS[parent]['transformations'].append('Extracted independent contract into '+newid)

p='shared-database-extension-contract'; b=ENTRIES[p][1]
t=re.search(r'For cached numeric preferences,.*?(?=\n\n|\Z)',b,re.S)[0]
move_paragraph(p,t,'numeric-preference-codec-normalization','Normalize numeric preferences at the codec boundary','storage','common',
'A persisted integral double returns as an integer and breaks a typed preference consumer.',
'Accept verified finite numeric representations at the read boundary while preserving validation and legitimate zero.',
'Applies when the resolved storage/codec actually changes numeric representation; it is not permission to coerce arbitrary values.',
'Rewriting the same numeric literal repeatedly does not fix a codec representation change.',
'Round-trip fractional and integral values, zero, missing data, strings, nonfinite numbers and out-of-range input.')

p='chunked-native-file-transfer';b=ENTRIES[p][1]
t=re.search(r'For record/page protocols,.*?(?=\n\n|\Z)',b,re.S)[0]
move_paragraph(p,t,'record-protocol-byte-budgets','Bound record pages by encoded bytes and explicit progress','integration','common',
'A record synchronization page exceeds the message ceiling or stalls after a short byte-limited page.',
'Budget encoded bytes including envelope overhead and treat exhaustion separately from page length.',
'Applies to framed record/page protocols; transport message limits, native bridge limits and database transaction limits are separate.',
'Do not drop oversized records, split an indivisible record silently, or infer end-of-data from a short page.',
'Test Unicode expansion, envelope overhead, one oversized record, a short nonterminal page and retries after partial acknowledged progress.')

p='database-backend-and-data-contract';b=ENTRIES[p][1]
t=re.search(r'A host\'s per-commit record limit.*?(?=\n\n|\Z)',b,re.S)[0]
move_paragraph(p,t,'batched-import-partial-commit','Make partial commit and resume explicit for batched imports','storage','common',
'An import exceeds a native store transaction limit or fails after earlier batches were committed.',
'Use bounded sequential batches and a durable progress policy rather than claiming whole-import rollback.',
'Applies when the actual store imposes per-commit limits. Whole-import atomicity needs a separate staging/publication design.',
'The last batch failing does not roll back earlier committed batches.',
'Fail before, during and after a batch commit; verify stable record IDs, conflict handling and idempotent resume without duplicate publication.')

# Remove an unrelated renderer-specific prerequisite inherited from a parent.
m,b=ENTRIES['shared-database-extension-contract']
a='A preferences/JSON store may be sufficient for small simple state, but it is not automatically equivalent to SQLite. A missing package implementation does not imply the OS cannot support SQLite.'
r='Requires a native extension and shared runtime that can access the same physical database with compatible schema, configuration and permissions.'
b=b.replace(a,r);m['applicability']=r;ENTRIES[m['id']]=(m,b)

CHECKS={
'flutter-engine-cache-coherence':'Rebuild from a clean compatible SDK cache, verify component identities/hashes and test release startup. Confirm the original first failure is gone without a semantic workaround.',
'shared-database-extension-contract':'Populate through the primary owner, read through the native extension, and verify identical records/schema. Cover an absent store, mismatched paths, open failure, migration and simultaneous access.',
'typed-preference-schema-migration':'Migrate valid legacy strings, ordinary JSON-looking text, explicit null, empty collections and invalid values. Inject publication failure and verify persisted bytes/caches remain consistent; test mixed-version peers.',
'transport-reply-watchdog':'Test one delayed ping with multiple subscribers, timeout followed by a buffered reply, a blackholed connection, busy application RPC and network-event bursts. Confirm one probe and honest shared failure.',
'calendar-file-handoff-completion':'Test valid/invalid ICS content, missing receiver, denied grant, cancellation and delayed receiver reading. Confirm UI reports handoff rather than an unmeasured inserted-event count.',
'external-deep-action-contract':'Test cold/warm receiver, encoded special characters and supplementary Unicode, unsupported routes, absent receiver and launch rejection. Confirm accepted launch is distinct from destination navigation.',
'scheduled-event-delivery-stages':'Fail after schedule consumption, runtime startup and submission. Test missing acknowledgement, diagnostic-persistence failure, duplicate delivery and invalid activation during ordinary launch.',
'gallery-image-format-preservation':'Save PNG, GIF and JPEG bytes with matching metadata; test invalid/truncated input, cancellation, native failure and cleanup failure. Compare gallery bytes and staging lifetime.',
'arkweb-synthetic-input-exclusions':'Exercise game gestures and recovery controls together; test pointer targets inside nested button elements and teardown while selection/write requests are pending.',
'native-form-current-data-provider':'Test the app closed across a day boundary, multiple form IDs, removal during refresh, missing database and schema upgrade. Confirm current payloads and closed native cursors/stores.',
'flutter-native-material-state':'Change material before shell readiness, recreate the shell, reject invalid stored values and simulate unsupported SDK/send failure. Verify desired and applied states stay distinct.',
'native-uri-streaming-read':'Measure memory during selection and ranged reads; test empty content, premature EOF, oversized chunks, changed content, invalid tokens, cancellation and detach during a pending read. Verify descriptors return to baseline.',
'startup-work-prerequisites':'Measure cold first-frame and subsequent interaction costs separately. Verify no consumer runs before required grants/storage, and that deferred work is cancelled or ignored after owner disposal.',
'bounded-upload-staging':'Measure memory for a large attachment and preview; test growth/truncation, same-size mutation, cancelled selection, stalled transfer and cleanup after failure/success.',
'resumable-upload-identity':'Disconnect after acknowledged writes and retry to the same target. Reject mismatched prefixes and changed local content; test draft removal, navigation, host switch and late completion.',
'rust-build-host-discovery':'Exercise a rustup-managed install, distribution shim, explicit CARGO_HOME/RUSTUP_HOME and missing managed directories; verify the actual compiler and unchanged explicit environment.',
'file-secret-store-guarantees':'Verify access controls, key/path validation, missing versus denied records, interrupted replacement and actual OH provider selection. Confirm no encryption claim is made without that mechanism.',
'flutter-player-async-teardown':'Test repeated dispose, late stream events, delayed/rejected native disposal and rapid replacement; verify one release and no replacement before required release completion.',
'flutter-player-surface-reconstruction':'Change source/output options rapidly, fail a rebuild, retry with no player and replace the controller after widget mount. Verify pause, position, speed and track rebinding.',
'flutter-native-video-output-selection':'Test texture and native-view outputs, delayed dimensions, controller replacement, in-app overlays, rapid entry/exit and disposal during reconstruction.',
'flutter-arkweb-page-zoom':'Vary application scale and page zoom independently and together; check taps/selection, existing root CSS zoom, navigation, reset to 1x, script removal and JavaScript-disabled fallback.',
'arkui-density-and-inset-layout':'Change density with unchanged physical window bounds, move/resize free windows, vary system bars/cutouts and recreate the UI owner. Confirm correct hit coordinates, no double insets and exact listener removal.',
}
for ident,checks in CHECKS.items():
    m,b=ENTRIES[ident]; b=re.sub(r'## Check it works\n\n.*', '## Check it works\n\n'+checks+'\n',b,flags=re.S)
    # Specific failure consequences already live in implementation; don't pad each atom identically.
    b=b.replace('Do not infer completion or portability from construction, a queued request, or a method name alone. Preserve the lifetime and failure distinctions above.', 'Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.')
    ENTRIES[ident]=(m,b)

# Index summaries should follow the narrowed entry after splitting other mechanisms away.
m,b=ENTRIES['database-backend-and-data-contract'];m['related'].extend(['typed-preference-schema-migration','shared-database-extension-contract']);ENTRIES[m['id']]=(m,b)
# Preserve explicit warnings about corrected source recipes.
for ident,note in {
'native-dependency-closure':'Did not promote unsafe empty native stubs or copied device-system libraries into the general solution; use complete dependencies or disable unavailable features.',
'qt-resource-registration-linkage':'Made symbol/linkage diagnosis conditional and retained the C++ helper alternative rather than prescribing unconditional macro deletion.',
'qt-optional-module-requirements':'Did not promote a historical inability to cross-build a module into a permanent platform prohibition.',
'input-driven-io-coalescing':'Replaced multiple one-shot timers with owned coalescing; omitted unproven symbol-resolution explanation.',
'windows-build-environment':'Scoped failure to observed shell/PATH formatting rather than asserting a universal Windows Node executable lookup limitation.',
}.items():PROVS[ident]['transformations'].append(note)

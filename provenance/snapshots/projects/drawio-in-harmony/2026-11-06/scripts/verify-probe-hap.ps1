param(
    [string]$HapPath = (Join-Path $PSScriptRoot '../artifacts/drawio-probe-api23-unsigned.hap'),
    [ValidateSet('unsigned','build-reported-signed')][string]$SignatureStatus = 'unsigned',
    [ValidateSet('probe','application')][string]$PayloadKind = 'probe'
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$migrationRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$resolvedHap = (Resolve-Path -LiteralPath $HapPath).Path
$runtime = Get-Content (Join-Path $migrationRoot 'validation/runtime-candidate.json') -Encoding utf8 -Raw | ConvertFrom-Json
$zip = [IO.Compression.ZipFile]::OpenRead($resolvedHap)
$checks = [Collections.Generic.List[object]]::new()
try {
    $required = @{
        'libs/arm64-v8a/libelectron.so' = 'ohos_hap/electron/libs/arm64-v8a/libelectron.so'
        'libs/arm64-v8a/libadapter.so' = 'ohos_hap/electron/libs/arm64-v8a/libadapter.so'
        'libs/arm64-v8a/libffmpeg.so' = 'ohos_hap/electron/libs/arm64-v8a/libffmpeg.so'
        'libs/arm64-v8a/libc++_shared.so' = 'ohos_hap/electron/libs/arm64-v8a/libc++_shared.so'
    }
    # electron.hnp is repacked by patch-hnp-permission.mjs (binaries get a .permission section),
    # so it is checked against that script's record, which itself verifies the upstream source.
    $hnpRecord = Get-Content (Join-Path $migrationRoot 'validation/electron-hnp-permission.json') -Encoding utf8 -Raw | ConvertFrom-Json
    $upstreamHnp = @($runtime.assets | Where-Object { $_.path -eq 'ohos_hap/hnp/arm64-v8a/electron.hnp' })[0]
    if ($hnpRecord.sourceSha256 -ne $upstreamHnp.sha256) { throw 'Patched HNP was not built from the recorded upstream HNP' }
    $hnpEntry = $zip.GetEntry('hnp/arm64-v8a/electron.hnp')
    if (-not $hnpEntry) { throw 'Missing required HAP entry: hnp/arm64-v8a/electron.hnp' }
    $stream = $hnpEntry.Open()
    $sha = [Security.Cryptography.SHA256]::Create()
    try { $actual = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant() }
    finally { $sha.Dispose(); $stream.Dispose() }
    if ($actual -ne $hnpRecord.outputSha256) { throw 'Runtime asset hash mismatch: hnp/arm64-v8a/electron.hnp' }
    $checks.Add([pscustomobject]@{ path = 'hnp/arm64-v8a/electron.hnp'; bytes = $hnpEntry.Length; sha256 = $actual; verified = $true })
    foreach ($asset in $runtime.assets) {
        $prefix = 'ohos_hap/web_engine/src/main/resources/resfile/'
        if ($asset.path.StartsWith($prefix)) {
            $required['resources/resfile/' + $asset.path.Substring($prefix.Length)] = $asset.path
        }
    }
    foreach ($name in $required.Keys) {
        $entry = $zip.GetEntry($name)
        if (-not $entry) { throw "Missing required HAP entry: $name" }
        $expected = @($runtime.assets | Where-Object { $_.path -eq $required[$name] })[0]
        $stream = $entry.Open()
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $actual = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant() }
        finally { $sha.Dispose(); $stream.Dispose() }
        if ($actual -ne $expected.sha256) { throw "Runtime asset hash mismatch: $name" }
        $checks.Add([pscustomobject]@{ path = $name; bytes = $entry.Length; sha256 = $actual; verified = $true })
    }
    if ($PayloadKind -eq 'application') {
        # The application ships as one app.asar (see stage-drawio-application.mjs); no loose sources.
        $payload = Get-Content (Join-Path $migrationRoot 'validation/application-payload.json') -Encoding utf8 -Raw | ConvertFrom-Json
        $asarEntry = $zip.GetEntry('resources/resfile/resources/app.asar')
        if (-not $asarEntry) { throw 'Payload archive missing: resources/resfile/resources/app.asar' }
        $stream = $asarEntry.Open()
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $actual = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant() }
        finally { $sha.Dispose(); $stream.Dispose() }
        if ($actual -ne $payload.asarSha256) { throw 'app.asar differs from the staged payload record' }
        $checks.Add([pscustomobject]@{ path = 'resources/resfile/resources/app.asar'; bytes = $asarEntry.Length; sha256 = $actual; verified = $true })
        $sources = @($zip.Entries | Where-Object { $_.FullName -match '\.(js|mjs|cjs|ts|jsx|tsx|java|py|c|cc|cpp|h)$' })
        if ($sources.Count -gt 0) { throw "Source files in HAP: $(($sources | Select-Object -First 5 | ForEach-Object FullName) -join ', ')" }
    }
    $payloadFiles = if ($PayloadKind -eq 'application') { @() } else { @('package.json', 'main.cjs', 'preload.cjs', 'renderer.js', 'index.html', 'style.css') }
    foreach ($name in $payloadFiles) {
        $entryName = 'resources/resfile/resources/app/' + $name
        $entry = $zip.GetEntry($entryName)
        if (-not $entry) { throw "Payload file missing: $entryName" }
        $stream = $entry.Open()
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $actual = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant() }
        finally { $sha.Dispose(); $stream.Dispose() }
        $sourceFolder = if ($PayloadKind -eq 'application') { 'application/' } else { 'probe/' }
        $expected = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $migrationRoot ($sourceFolder + $name))).Hash.ToLowerInvariant()
        if ($actual -ne $expected) { throw "Payload source differs from HAP: $entryName" }
        $checks.Add([pscustomobject]@{ path = $entryName; bytes = $entry.Length; sha256 = $actual; verified = $true })
    }
    $abc = $zip.GetEntry('ets/modules.abc')
    if (-not $abc -or $abc.Length -eq 0) { throw 'Compiled ArkTS payload missing' }
    $reader = [IO.StreamReader]::new($zip.GetEntry('module.json').Open())
    try { $module = $reader.ReadToEnd() | ConvertFrom-Json } finally { $reader.Dispose() }
    if ($module.app.targetAPIVersion -ne 60100023 -or $module.app.minAPIVersion -ne 60100023) {
        throw 'Unexpected target/min API version'
    }
    if ($module.module.hnpPackages[0].package -ne 'electron.hnp') { throw 'HNP declaration missing' }
    $zip.Entries | Select-Object FullName,Length | ConvertTo-Json | Set-Content (Join-Path $migrationRoot 'validation/hap-contents.json') -Encoding utf8
} finally { $zip.Dispose() }
$evidence = [pscustomobject]@{
    observedAt = [DateTime]::UtcNow.ToString('o')
    artifact = $resolvedHap
    bytes = (Get-Item -LiteralPath $resolvedHap).Length
    sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $resolvedHap).Hash.ToLowerInvariant()
    bundleName = $module.app.bundleName
    targetAPIVersion = $module.app.targetAPIVersion
    minAPIVersion = $module.app.minAPIVersion
    compileSdkVersion = $module.app.compileSdkVersion
    signature = $SignatureStatus
    deviceValidation = 'pending'
    payloadKind = $PayloadKind
    checks = $checks.ToArray()
}
$evidenceName = if ($SignatureStatus -eq 'unsigned') { 'hap-validation.json' } else { 'hap-signed-validation.json' }
if ($PayloadKind -eq 'application') { $evidenceName = 'application-' + $evidenceName }
$evidence | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $migrationRoot ('validation/' + $evidenceName)) -Encoding utf8
$evidence | Select-Object artifact,bytes,sha256,bundleName,targetAPIVersion,signature,deviceValidation | ConvertTo-Json

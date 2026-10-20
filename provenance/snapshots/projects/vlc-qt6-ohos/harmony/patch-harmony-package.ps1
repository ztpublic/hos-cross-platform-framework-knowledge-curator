param(
    [Parameter(Mandatory = $true)][string]$PackageRoot,
    [Parameter(Mandatory = $true)][string]$VlcUsrRoot,
    [Parameter(Mandatory = $true)][string]$QtPlugin,
    [string]$QtPlatformPlugin
)

$ErrorActionPreference = 'Stop'
$libDir = Join-Path $PackageRoot 'entry\libs\arm64-v8a'
if (-not (Test-Path $libDir)) { throw 'Qt HarmonyOS deployment output is incomplete.' }

$modulePath = Join-Path $PackageRoot 'entry\src\main\module.json5'
$entryStringsPath = Join-Path $PackageRoot 'entry\src\main\resources\base\element\string.json'
$appStringsPath = Join-Path $PackageRoot 'AppScope\resources\base\element\string.json'
$exportModulesPath = Join-Path $PackageRoot 'entry\src\main\ets\qability\OhosExportModules.ts'
$qAbilityPath = Join-Path $PackageRoot 'entry\src\main\ets\qability\QAbility.ets'
$iconSourcePath = Join-Path $PSScriptRoot '..\share\vlc512x512.png'
if (-not (Test-Path $modulePath) -or -not (Test-Path $entryStringsPath) `
    -or -not (Test-Path $appStringsPath) -or -not (Test-Path $qAbilityPath)) {
    throw 'Qt deployment output is missing HarmonyOS manifest or resource files.'
}

function Enable-FileWrite {
    param([Parameter(Mandatory = $true)][string]$Path)
    if (Test-Path -LiteralPath $Path) {
        $item = Get-Item -LiteralPath $Path
        if ($item.IsReadOnly) { $item.IsReadOnly = $false }
    }
}

@($modulePath, $entryStringsPath, $appStringsPath, $exportModulesPath, $qAbilityPath) |
    ForEach-Object { Enable-FileWrite -Path $_ }

function Set-ResourceString {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Value
    )
    $resources = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    $resources.string = @($resources.string | Where-Object { $_.name -ne $Name })
    $resources.string += [pscustomobject]@{ name = $Name; value = $Value }
    $resources | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $Path -Encoding utf8
}

# Media Want association, background playback declaration, and AVSession's
# required background-running permission. harmonydeployqt regenerates this
# manifest on every bundle, so apply the changes after each deployment pass.
$module = Get-Content -LiteralPath $modulePath -Raw
if ($module -notmatch '"backgroundModes"') {
    $backgroundModes = @'
"isolationProcess": true,
        "backgroundModes": [
          "audioPlayback"
        ],
'@
    $module = $module.Replace('"isolationProcess": true,', $backgroundModes.TrimEnd("`r", "`n"))
}
if ($module -notmatch '"linkFeature"\s*:\s*"FileOpen"') {
    $mediaUris = @'
"type": "video/*", "linkFeature": "FileOpen", "maxFileSupported": 10
              },
              {
                "scheme": "file", "type": "audio/*", "linkFeature": "FileOpen", "maxFileSupported": 10
              },
              {
                "scheme": "file", "utd": "general.audio", "linkFeature": "FileOpen", "maxFileSupported": 10
              },
              {
                "scheme": "file",
                "utd": "general.text",
'@
    $module = $module.Replace('"utd": "general.text",', $mediaUris.TrimEnd("`r", "`n"))
}
if ($module -notmatch 'ohos.permission.KEEP_BACKGROUND_RUNNING') {
    $permissionPattern = '(?s)(\{\s*"name"\s*:\s*"ohos\.permission\.GET_WIFI_INFO"\s*\})(\s*\]\s*\}\s*\})'
    if (-not [regex]::IsMatch($module, $permissionPattern)) {
        throw 'Unable to locate the end of requestPermissions in module.json5.'
    }
    $module = [regex]::Replace($module, $permissionPattern, {
        param($match)
        $extraPermissions = @'
,
        { "name": "ohos.permission.GET_NETWORK_INFO" },
        { "name": "ohos.permission.VIBRATE" },
        {
            "name": "ohos.permission.KEEP_BACKGROUND_RUNNING",
            "reason": "$string:reason_keep_background_running",
            "usedScene": { "abilities": ["QAbility"], "when": "always" }
        }
'@
        return $match.Groups[1].Value + $extraPermissions.TrimEnd("`r", "`n") + $match.Groups[2].Value
    }, 1)
}
Set-Content -LiteralPath $modulePath -Value $module -Encoding utf8

Set-ResourceString -Path $entryStringsPath -Name 'reason_keep_background_running' `
    -Value 'Used to keep media playback running while VLC is in the background.'
Set-ResourceString -Path $entryStringsPath -Name 'QAbility_label' -Value 'VLC'
Set-ResourceString -Path $appStringsPath -Name 'app_name' -Value 'VLC'

# Replace all Qt template launcher/start-window images with VLC's cone.
if (-not (Test-Path -LiteralPath $iconSourcePath)) {
    throw "VLC launcher icon not found: $iconSourcePath"
}
$iconTargets = @(
    (Join-Path $PackageRoot 'AppScope\resources\base\media\app_icon.png'),
    (Join-Path $PackageRoot 'entry\src\main\resources\base\media\foreground.png'),
    (Join-Path $PackageRoot 'entry\src\main\resources\base\media\startIcon.png')
)
foreach ($iconTarget in $iconTargets) {
    Enable-FileWrite -Path $iconTarget
    Copy-Item -LiteralPath $iconSourcePath -Destination $iconTarget -Force
}

# Keep the generated ArkTS registry consistent with the reference media app.
if (Test-Path -LiteralPath $exportModulesPath) {
    $exports = Get-Content -LiteralPath $exportModulesPath -Raw
    if ($exports -notmatch '__kit__BackgroundTasksKit__backgroundTaskManager') {
        $imports = @'
import lazy { backgroundTaskManager as __kit__BackgroundTasksKit__backgroundTaskManager } from '@kit.BackgroundTasksKit';
import lazy { wantAgent as __kit__AbilityKit__wantAgent } from '@kit.AbilityKit';
'@
        $exports = $imports.TrimEnd("`r", "`n") + "`r`n" + $exports
        $factories = @'
    '@kit.BackgroundTasksKit.backgroundTaskManager': () => __kit__BackgroundTasksKit__backgroundTaskManager,
    '@kit.AbilityKit.wantAgent': () => __kit__AbilityKit__wantAgent,
'@
        $exports = $exports.Replace('  return {', "  return {`r`n$($factories.TrimEnd("`r", "`n"))")
        Set-Content -LiteralPath $exportModulesPath -Value $exports -Encoding utf8
    }
}

# Qt forwards Want lifecycle events internally but does not expose Want.uri to
# this VLC 3 Qt interface. QAbility appends URIs to filesDir and the native Qt
# bridge consumes the short-lived queue on the GUI thread.
$qAbility = Get-Content -LiteralPath $qAbilityPath -Raw
if ($qAbility -notmatch 'vlc-open-wants\.txt') {
    $qAbility = $qAbility.Replace(
        "import Want from '@ohos.app.ability.Want';",
        "import Want from '@ohos.app.ability.Want';`r`nimport { fileIo as fs } from '@kit.CoreFileKit';")
    $wantQueueMethod = @'
  private forwardWantToQt(want: Want): void {
    if (!want.uri) {
      return;
    }
    const queue = fs.openSync(this.context.filesDir + '/vlc-open-wants.txt',
      fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE | fs.OpenMode.APPEND);
    fs.writeSync(queue.fd, want.uri + '\n');
    fs.closeSync(queue);
    hilog.info(LOG_DOMAIN, LOG_TAG, 'QAbility forwarded Want URI: ' + want.uri);
  }

'@
    $qAbility = $qAbility.Replace(
        '  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {',
        "$wantQueueMethod  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {")
    $qAbility = $qAbility.Replace(
        '    QAbilityStage.initQtAppContextIfNeeded(this.context.getApplicationContext());',
        "    this.forwardWantToQt(want);`r`n    QAbilityStage.initQtAppContextIfNeeded(this.context.getApplicationContext());")
    $qAbility = $qAbility.Replace(
        "    hilog.info(LOG_DOMAIN, LOG_TAG, 'QAbility::onNewWant(): want.parameters: ' + JSON.stringify(want.parameters));",
        "    hilog.info(LOG_DOMAIN, LOG_TAG, 'QAbility::onNewWant(): want.parameters: ' + JSON.stringify(want.parameters));`r`n    this.forwardWantToQt(want);")
    Set-Content -LiteralPath $qAbilityPath -Value $qAbility -Encoding utf8
}

if ($QtPlatformPlugin) {
    if (-not (Test-Path -LiteralPath $QtPlatformPlugin)) {
        throw "Patched Qt platform plugin not found: $QtPlatformPlugin"
    }
    Copy-Item -LiteralPath $QtPlatformPlugin -Destination (Join-Path $libDir 'libqohos.so') -Force
}

$vlcPluginSource = Join-Path $VlcUsrRoot 'vlc\arm64-v8a\lib\vlc\plugins'
$vlcPluginTarget = Join-Path $libDir 'vlc\plugins'
New-Item -ItemType Directory -Force -Path $vlcPluginTarget | Out-Null
Get-ChildItem -LiteralPath $vlcPluginSource -Force | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $vlcPluginTarget -Recurse -Force
}

$guiPluginDir = Join-Path $vlcPluginTarget 'gui'
New-Item -ItemType Directory -Force -Path $guiPluginDir | Out-Null
Copy-Item -LiteralPath $QtPlugin -Destination (Join-Path $guiPluginDir 'libqt_plugin.so') -Force

# VLC's Qt interface owns its QApplication on the VLC interface thread while
# the shared-library main function waits for libvlc to exit.  Qt for HarmonyOS
# otherwise treats that intentional wait as a frozen Qt main thread.
$abilityStagePath = Join-Path $PackageRoot 'entry\src\main\ets\qabilitystage\QAbilityStage.ets'
$abilityStage = Get-Content -LiteralPath $abilityStagePath -Raw
$acceptWantMarker = '  onAcceptWant(want: Want): string {'
$watchdogSetting = "    want.parameters = want.parameters ?? {};`n    want.parameters['io.qt.watchdogEnabled'] = false;"
if (-not $abilityStage.Contains("want.parameters['io.qt.watchdogEnabled'] = false;")) {
    if (-not $abilityStage.Contains($acceptWantMarker)) {
        throw 'Unable to locate QAbilityStage.onAcceptWant in the Qt HarmonyOS template.'
    }
    $abilityStage = $abilityStage.Replace($acceptWantMarker, $acceptWantMarker + "`n" + $watchdogSetting)
    Set-Content -LiteralPath $abilityStagePath -Value $abilityStage -NoNewline
}

Get-ChildItem -LiteralPath $VlcUsrRoot -File -Recurse | Where-Object {
    $_.FullName -match '\\arm64-v8a\\lib\\' -and $_.Name -match '\.so(\.|$)'
} | ForEach-Object {
    if ($_.FullName -notlike "$vlcPluginSource*") {
        Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $libDir $_.Name) -Force
    }
}

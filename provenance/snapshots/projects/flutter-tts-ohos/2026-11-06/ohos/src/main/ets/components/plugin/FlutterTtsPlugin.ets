import {
  FlutterPlugin,
  FlutterPluginBinding,
  MethodCall,
  MethodCallHandler,
  MethodChannel,
  MethodResult,
} from '@ohos/flutter_ohos';
import { AbilityAware, AbilityPluginBinding } from '@ohos/flutter_ohos';
import { common, UIAbility } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { textToSpeech } from '@kit.CoreSpeechKit';

// Person (voice) constants for HarmonyOS TTS
const PERSON_ZH_LING_XIAO_SHAN: number = 13; // 聆小珊 female Chinese (default)
const PERSON_EN_LAURA: number = 8;           // Laura female English

/** FlutterTtsPlugin for HarmonyOS **/
export default class FlutterTtsPlugin implements FlutterPlugin, MethodCallHandler, AbilityAware {
  private methodChannel: MethodChannel | null = null;
  private engine?: textToSpeech.TextToSpeechEngine;
  private language: string = 'zh-CN';
  private person: number = PERSON_ZH_LING_XIAO_SHAN;
  private speechRate: number = 1.0;
  private volume: number = 1.0;
  private pitch: number = 1.0;
  private awaitSpeakCompletion: boolean = false;
  private awaitSynthCompletion: boolean = false;
  private isSpeaking: boolean = false;
  private currentRequestId: string = '';
  private queueMode: number = 1; // 0 = queued, 1 = preemptive
  private voices: Array<textToSpeech.VoiceInfo> = [];
  private speakResult: MethodResult | null = null;
  private static _context: common.Context | null = null;
  private static _uiContext: common.UIAbilityContext | null = null;

  constructor() {
  }

  static get uiContext(): common.UIAbilityContext | null {
    return FlutterTtsPlugin._uiContext;
  }

  static get context(): common.Context | null {
    return FlutterTtsPlugin._context;
  }

  get uiContext(): common.UIAbilityContext | null {
    return FlutterTtsPlugin._uiContext;
  }

  getUniqueClassName(): string {
    return "FlutterTtsPlugin"
  }

  onAttachedToAbility(binding: AbilityPluginBinding): void {
    FlutterTtsPlugin._uiContext = binding.getAbility().context;
  }

  onDetachedFromAbility(): void {
  }

  onAttachedToEngine(binding: FlutterPluginBinding): void {
    this.methodChannel = new MethodChannel(binding.getBinaryMessenger(), "flutter_tts");
    this.methodChannel.setMethodCallHandler(this);
    this.initEngine();
  }

  onDetachedFromEngine(binding: FlutterPluginBinding): void {
    if (this.methodChannel != null) {
      this.methodChannel.setMethodCallHandler(null);
      this.methodChannel = null;
    }
    if (this.engine) {
      this.engine.setListener(null);
      this.engine.shutdown();
      this.engine = undefined;
    }
  }

  private async initEngine(): Promise<void> {
    const createParams: textToSpeech.CreateEngineParams = {
      language: this.language,
      online: 1, // offline mode
      person: this.person,
      extraParams: {
        'isBackStage': true
      }
    };

    try {
      this.engine = await textToSpeech.createEngine(createParams);
      this.setupListener();
      this.loadVoices();
    } catch (error) {
      console.error('FlutterTtsPlugin: Failed to create TTS engine', JSON.stringify(error));
    }
  }

  private setupListener(): void {
    if (!this.engine) return;

    const listener: textToSpeech.SpeakListener = {
      onStart: (requestId: string, response: textToSpeech.StartResponse): void => {
        if (requestId === this.currentRequestId) {
          this.isSpeaking = true;
          this.methodChannel?.invokeMethod('speak.onStart', null);
        }
      },
      onData: (requestId: string, audio: ArrayBuffer, response: textToSpeech.SynthesisResponse): void => {
        // Data callback for audio stream, not used
      },
      onStop: (requestId: string, response: textToSpeech.StopResponse): void => {
        if (requestId === this.currentRequestId) {
          this.isSpeaking = false;
          if (this.awaitSpeakCompletion && this.speakResult != null) {
            this.speakResult.success(1);
            this.speakResult = null;
          }
          this.methodChannel?.invokeMethod('speak.onCancel', null);
        }
      },
      onComplete: (requestId: string, response: textToSpeech.CompleteResponse): void => {
        if (requestId === this.currentRequestId) {
          // type: 0 = synthesis ended, 1 = playback ended
          if (response.type === 1) {
            this.isSpeaking = false;
            if (this.awaitSpeakCompletion && this.speakResult != null) {
              this.speakResult.success(1);
              this.speakResult = null;
            }
            this.methodChannel?.invokeMethod('speak.onComplete', null);
          }
        }
      },
      onError: (requestId: string, errorCode: number, errorMessage: string): void => {
        if (requestId === this.currentRequestId) {
          this.isSpeaking = false;
          if (this.awaitSpeakCompletion && this.speakResult != null) {
            this.speakResult.success(0);
            this.speakResult = null;
          }
          this.methodChannel?.invokeMethod('speak.onError', `${errorMessage} (code: ${errorCode})`);
        }
      }
    };

    this.engine.setListener(listener);
  }

  private async loadVoices(): Promise<void> {
    if (!this.engine) return;

    try {
      const query: textToSpeech.VoiceQuery = {
        requestId: this.generateRequestId(),
        online: 1
      };
      this.voices = await this.engine.listVoices(query);
    } catch (error) {
      console.error('FlutterTtsPlugin: Failed to load voices', JSON.stringify(error));
    }
  }

  private generateRequestId(): string {
    return 'flutter_tts_' + Date.now().toString() + '_' + Math.random().toString(36).substring(2, 8);
  }

  onMethodCall(call: MethodCall, result: MethodResult): void {
    switch (call.method) {
      case 'speak':
        this.handleSpeak(call, result);
        break;
      case 'awaitSpeakCompletion':
        this.awaitSpeakCompletion = (call.args as boolean) ?? false;
        result.success(1);
        break;
      case 'awaitSynthCompletion':
        this.awaitSynthCompletion = (call.args as boolean) ?? false;
        result.success(1);
        break;
      case 'stop':
        this.handleStop(result);
        break;
      case 'pause':
        this.handleStop(result);
        break;
      case 'setLanguage':
        this.handleSetLanguage(call, result);
        break;
      case 'setSpeechRate':
        this.handleSetSpeechRate(call, result);
        break;
      case 'setVolume':
        this.handleSetVolume(call, result);
        break;
      case 'setPitch':
        this.handleSetPitch(call, result);
        break;
      case 'getLanguages':
        this.handleGetLanguages(result);
        break;
      case 'getVoices':
        this.handleGetVoices(result);
        break;
      case 'setVoice':
        this.handleSetVoice(call, result);
        break;
      case 'clearVoice':
        this.handleClearVoice(result);
        break;
      case 'isLanguageAvailable':
        this.handleIsLanguageAvailable(call, result);
        break;
      case 'getSpeechRateValidRange':
        this.handleGetSpeechRateValidRange(result);
        break;
      case 'getMaxSpeechInputLength':
        result.success(10000);
        break;
      case 'setQueueMode':
        const mode: number = (call.args as number) ?? 0;
        this.queueMode = mode;
        result.success(1);
        break;
      case 'synthesizeToFile':
        this.handleSynthesizeToFile(call, result);
        break;
      case 'setEngine':
        // Android only, no-op
        result.success(1);
        break;
      case 'getEngines':
        // Android only
        result.success([]);
        break;
      case 'getDefaultEngine':
        // Android only
        result.success('');
        break;
      case 'getDefaultVoice':
        // Android only
        result.success(null);
        break;
      case 'isLanguageInstalled':
        // Android only
        result.success(true);
        break;
      case 'areLanguagesInstalled':
        // Android only
        result.success({});
        break;
      case 'setSilence':
        // Android only, no-op
        result.success(1);
        break;
      case 'setAudioAttributesForNavigation':
        // Android only, no-op
        result.success(1);
        break;
      case 'setIosAudioCategory':
        // iOS only, no-op
        result.success(1);
        break;
      case 'setSharedInstance':
        // iOS only, no-op
        result.success(1);
        break;
      case 'autoStopSharedSession':
        // iOS only, no-op
        result.success(1);
        break;
      default:
        result.notImplemented();
        break;
    }
  }

  private handleSpeak(call: MethodCall, result: MethodResult): void {
    if (!this.engine) {
      result.error('Engine not initialized', 'TTS engine is not initialized', null);
      return;
    }

    let text: string;
    const args: ESObject = call.args;
    if (typeof args === 'string') {
      text = args;
    } else if (typeof args === 'object' && args != null) {
      text = (args as Record<string, string>)['text'] ?? '';
    } else {
      text = '';
    }

    if (!text || text.length === 0) {
      result.success(0);
      return;
    }

    if (this.isSpeaking && this.queueMode === 0) {
      result.success(0);
      return;
    }

    this.currentRequestId = this.generateRequestId();

    try {
      const speakParams: textToSpeech.SpeakParams = {
        requestId: this.currentRequestId,
        extraParams: {
          'speed': this.speechRate,
          'volume': this.volume,
          'pitch': this.pitch,
          'languageContext': this.language.replace('-', '_'),
          'audioType': 'pcm',
          'playType': 1,
          'soundChannel': 3,
          'queueMode': this.queueMode
        }
      };

      this.engine.speak(text, speakParams);

      if (this.awaitSpeakCompletion && this.queueMode === 0) {
        this.speakResult = result;
      } else {
        result.success(1);
      }
    } catch (error) {
      result.error('Speak failed', JSON.stringify(error), null);
    }
  }

  private handleStop(result: MethodResult): void {
    if (this.engine && this.isSpeaking) {
      this.engine.stop();
    }
    this.isSpeaking = false;
    if (this.speakResult != null) {
      this.speakResult.success(1);
      this.speakResult = null;
    }
    result.success(1);
  }

  private handleSetLanguage(call: MethodCall, result: MethodResult): void {
    const lang: string = call.args as string;
    const isAvailable = this.checkLanguageAvailable(lang);

    if (isAvailable) {
      this.language = lang;
      if (lang.startsWith('zh')) {
        this.person = PERSON_ZH_LING_XIAO_SHAN;
      } else if (lang.startsWith('en')) {
        this.person = PERSON_EN_LAURA;
      }
      if (this.engine) {
        this.engine.shutdown();
        this.engine = undefined;
      }
      this.initEngine();
      result.success(1);
    } else {
      result.success(0);
    }
  }

  private handleSetSpeechRate(call: MethodCall, result: MethodResult): void {
    const rate: number = call.args as number;
    if (rate >= 0.5 && rate <= 2.0) {
      this.speechRate = rate;
      result.success(1);
    } else {
      result.success(0);
    }
  }

  private handleSetVolume(call: MethodCall, result: MethodResult): void {
    const vol: number = call.args as number;
    if (vol >= 0.0 && vol <= 2.0) {
      this.volume = vol;
      result.success(1);
    } else {
      result.success(0);
    }
  }

  private handleSetPitch(call: MethodCall, result: MethodResult): void {
    const p: number = call.args as number;
    if (p >= 0.5 && p <= 2.0) {
      this.pitch = p;
      result.success(1);
    } else {
      result.success(0);
    }
  }

  private handleGetLanguages(result: MethodResult): void {
    const langs: string[] = [];
    for (const voice of this.voices) {
      if (voice.language && !langs.includes(voice.language)) {
        langs.push(voice.language);
      }
    }
    result.success(langs);
  }

  private handleGetVoices(result: MethodResult): void {
    const voiceList: Array<Record<string, string>> = [];
    for (const voice of this.voices) {
      const voiceMap: Record<string, string> = {
        'name': voice.person?.toString() ?? '',
        'locale': voice.language ?? ''
      };
      if (voice.gender) {
        voiceMap['gender'] = voice.gender;
      }
      if (voice.style) {
        voiceMap['style'] = voice.style;
      }
      if (voice.description) {
        voiceMap['description'] = voice.description;
      }
      voiceList.push(voiceMap);
    }
    result.success(voiceList);
  }

  private handleSetVoice(call: MethodCall, result: MethodResult): void {
    const voiceMap: Record<string, string> = call.args as Record<string, string>;
    const name = voiceMap['name'];
    const locale = voiceMap['locale'];

    if (name) {
      const personNum = parseInt(name, 10);
      if (!isNaN(personNum)) {
        this.person = personNum;
        if (locale) {
          this.language = locale;
        }
        if (this.engine) {
          this.engine.shutdown();
          this.engine = undefined;
        }
        this.initEngine();
        result.success(1);
        return;
      }
    }

    if (locale) {
      this.language = locale;
      if (locale.startsWith('zh')) {
        this.person = PERSON_ZH_LING_XIAO_SHAN;
      } else if (locale.startsWith('en')) {
        this.person = PERSON_EN_LAURA;
      }
      if (this.engine) {
        this.engine.shutdown();
        this.engine = undefined;
      }
      this.initEngine();
      result.success(1);
      return;
    }

    result.success(0);
  }

  private handleClearVoice(result: MethodResult): void {
    this.person = PERSON_ZH_LING_XIAO_SHAN;
    result.success(1);
  }

  private handleIsLanguageAvailable(call: MethodCall, result: MethodResult): void {
    const lang: string = call.args as string;
    result.success(this.checkLanguageAvailable(lang));
  }

  private handleGetSpeechRateValidRange(result: MethodResult): void {
    const range: Record<string, string> = {
      'min': '0.5',
      'normal': '1.0',
      'max': '2.0',
      'platform': 'ohos'
    };
    result.success(range);
  }

  private handleSynthesizeToFile(call: MethodCall, result: MethodResult): void {
    // HarmonyOS CoreSpeechKit does not support synthesize to file directly
    result.notImplemented();
  }

  private checkLanguageAvailable(language: string): boolean {
    if (!language || language.length === 0) return false;

    const normalizedLang = language.toLowerCase().replace('_', '-');
    if (normalizedLang.startsWith('zh') || normalizedLang.startsWith('en')) {
      for (const voice of this.voices) {
        if (voice.language && voice.language.toLowerCase().startsWith(normalizedLang.split('-')[0])) {
          return true;
        }
      }
      return true;
    }
    return false;
  }
}

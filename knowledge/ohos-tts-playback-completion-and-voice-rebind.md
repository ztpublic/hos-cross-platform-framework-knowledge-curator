---
{
  "schema_version": 1,
  "id": "ohos-tts-playback-completion-and-voice-rebind",
  "title": "Use playback completion and rebuild the engine for OH TTS voices",
  "topic_key": "media.ohos-tts-playback-completion-and-voice-rebind",
  "area": "media",
  "scope": "common",
  "frameworks": [],
  "summary": "Treat CoreSpeechKit onComplete as finished only when response.type is 1, and shutdown and recreate the engine when language or voice changes.",
  "when_to_use": "A speak-completed callback fires too early, while speech is still playing, and changing language or voice has no effect.",
  "applicability": "CoreSpeechKit textToSpeech, reviewed in a Flutter plugin. The type meanings come from a source comment. The API is not available on every product, so gate it. Synthesis to file was not implemented in the reviewed plugin.",
  "keywords": [
    "textToSpeech",
    "CoreSpeechKit",
    "onComplete",
    "response.type",
    "createEngine",
    "shutdown",
    "voice change",
    "speak completion",
    "TTS"
  ],
  "related": [
    "capability-gated-backend-initialization",
    "audio-session-concurrency-policy",
    "system-lyric-session-and-privacy"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Use playback completion and rebuild the engine for OH TTS voices

## When to use

A speak-completed callback fires too early, while speech is still playing, and changing language or voice has no effect.

## Recommended approach

Treat CoreSpeechKit onComplete as finished only when response.type is 1, and shutdown and recreate the engine when language or voice changes.

## Implementation steps

1. Set a listener with onStart, onStop, onComplete and onError, and ignore events whose requestId is not the current request.
2. In onComplete, finish only for type 1, as type 0 is reported as synthesis end.
3. Settle the pending result in onStop and onError as well, or callers wait forever.
4. On a language or voice change, call engine.shutdown and then textToSpeech.createEngine, as the speaker is fixed at creation.
5. Await re-creation before the next speak. The reviewed plugin does not, so a speak immediately after a change can race.

## Conditions and alternatives

Queue or reject speak requests during re-initialization. Check engine availability first and offer a non-speech fallback.

## Common mistakes

Treating every onComplete as the end. Leaving results unresolved on stop. Reusing a shut-down engine.

## Check it works

Speak long and short text, stop in mid-speech, change voice immediately before speaking, and confirm completion timing against audio.

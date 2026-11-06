---
{
  "schema_version": 1,
  "id": "qt5-ohos-foreign-gl-video-window",
  "title": "Render video through a raw OpenGL QWindow on Qt 5 OH",
  "topic_key": "media.qt5-ohos-foreign-gl-video-window",
  "area": "media",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "If QMediaPlayer does not work on your Qt 5 OH build, drive a player library's GL renderer from a QWindow of OpenGLSurface type and embed it with createWindowContainer.",
  "when_to_use": "QVideoWidget or QMediaPlayer playback fails on Qt 5 OH, and QOpenGLWidget-based video does not display.",
  "applicability": "Qt 5 OH with a third-party player SDK offering a GL render API. The backend statement is a build-file comment on one port. Qt 6 has a separate texture path; see the related entry. Player SDK licensing is your responsibility.",
  "keywords": [
    "QVideoWidget",
    "QMediaPlayer",
    "AVPlayer backend",
    "QOpenGLContext",
    "createWindowContainer",
    "OpenGLSurface",
    "setRenderAPI",
    "isExposed",
    "MDK"
  ],
  "related": [
    "qt6-ohos-native-video-texture",
    "video-surface-backend-contract",
    "qt-opengl-texture-completeness"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Render video through a raw OpenGL QWindow on Qt 5 OH

## When to use

QVideoWidget or QMediaPlayer playback fails on Qt 5 OH, and QOpenGLWidget-based video does not display.

## Recommended approach

If QMediaPlayer does not work on your Qt 5 OH build, drive a player library's GL renderer from a QWindow of OpenGLSurface type and embed it with createWindowContainer.

## Implementation steps

1. Set an OpenGL ES 2 default with QSurfaceFormat::setDefaultFormat before any window exists.
2. Create a QWindow with setSurfaceType(OpenGLSurface) and your own QOpenGLContext.
3. Give the player a render API whose getProcAddress goes through the context, then size its surface from the window.
4. Embed with QWidget::createWindowContainer for layout.
5. Render on each frame callback without gating on isExposed, since the source says the QPA may not set it after a resize.

## Conditions and alternatives

Container windows stack above sibling widgets, so overlays need their own handling. A C ABI, as with ctypes, can expose this to Python apps.

## Common mistakes

Setting the format after windows exist. Using QOpenGLWidget on this path. Gating on isExposed and leaving a black video after a resize.

## Check it works

Play, seek and resize, enter fullscreen, hide and restore, and destroy the window during playback. Check for leaks over repeated opens.

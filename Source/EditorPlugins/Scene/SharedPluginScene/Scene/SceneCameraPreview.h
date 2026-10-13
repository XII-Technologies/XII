/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/World/Declarations.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

class xiiWorld;
class xiiView;
class xiiCameraComponent;

/// A scene viewport's selected-camera preview. Owns an independent world-local view, camera and
/// 256x144 render target; pipeline definitions are shared through the viewport's resource handle.
/// Uses the regular native rendering pipeline and debug overlay, without an extractor or global
/// render-target resource. The overlay samples the most recently rendered preview (initially black).
/// Must be destroyed before its world. Call SuspendRendering() when consuming the viewport's redraw.
class XII_SHAREDPLUGINSCENE_DLL xiiSceneCameraPreview
{
public:
  explicit xiiSceneCameraPreview(xiiWorld& world);
  ~xiiSceneCameraPreview();

  XII_DISALLOW_COPY_AND_ASSIGN(xiiSceneCameraPreview);

  /// Updates from an active camera in the same world and queues its overlay in an Editor view.
  /// Camera usage None is supported. Stereo cameras use a 45-degree mono perspective preview.
  /// Failure suspends the preview. Acquires the world's write marker internally.
  xiiResult Update(const xiiView& editorView, const xiiCameraComponent& camera);
  void      SuspendRendering();

  /// Non-owning view; caller must hold the world's access marker for the entire use.
  xiiView* GetView() const;

private:
  xiiResult EnsureView();

  xiiWorld&                   m_World;
  xiiCamera                   m_Camera;
  xiiViewHandle               m_hView;
  xiiComponentHandle          m_hCamera;
  xiiSharedPtr<xiiGALTexture> m_pTarget;
};

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

class xiiSceneShapeIconPicking;

/// Editor billboards implemented as a regular resource-defined pipeline pass. No sprite extractor,
/// render component or document pointer is required. Size is in world units, capped in screen pixels.
/// Inputs: Color (display-linear, single-sample), SceneDepth (native scene depth, single-sample).
/// Outputs: Color, Picking (R32UInt; zero = background, n = GetIcons()[n - 1]).
/// Picking and color use identical alpha/depth coverage. Picking IDs belong to this Build only;
/// asynchronous consumers must capture the matching handles before the next Build.
/// Disabled/non-Editor views pass through Color and expose no Picking output or icons.
/// Parameter Editor.ShapeIcons overrides the default enabled state. Build requires world read access.
class XII_SHAREDPLUGINSCENE_DLL xiiSceneShapeIconPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSceneShapeIconPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_InputColor;
  xiiRenderPipelineTextureInputPin  m_InputSceneDepth;
  xiiRenderPipelineTextureOutputPin m_OutputColor;
  xiiRenderPipelineTextureOutputPin m_OutputPicking{xiiGALResourceFormat::R32UInt, true};

  xiiSceneShapeIconPass();
  ~xiiSceneShapeIconPass() override;

  struct PickingResult
  {
    xiiGameObjectHandle m_hObject;
    xiiComponentHandle  m_hComponent;
    xiiVec3             m_vWorldPosition = xiiVec3::MakeZero();
  };
  struct Icon
  {
    xiiGameObjectHandle        m_hObject;
    xiiComponentHandle         m_hComponent;
    xiiTexture2DResourceHandle m_hTexture;
    xiiVec3                    m_vWorldPosition;
    xiiVec3                    m_vScreenPosition; ///< Normalized, top-left origin; z matches the view projection.
    xiiVec2                    m_vScreenSize;     ///< Normalized width and height.
    xiiColor                   m_Color;
    float                      m_fCameraDepth = 0; ///< Distance along camera forward; used for compositing order.
  };

  xiiResult Build(xiiRenderPipelinePassContext& context) override;

  /// Borrowed until the next Build. Far-to-near order, with object identity breaking depth ties.
  xiiArrayPtr<const Icon> GetIcons() const { return m_Icons; }

  /// Nonblocking queries against the newest completed GPU image. Success with an invalid object
  /// means background; failure means unavailable/outdated capture or invalid coordinates. Results
  /// are cleared first. Caller holds world read access at a CPU frame boundary. Validates live
  /// component/object identities, visibility and unchanged object position before returning a hit.
  /// Editor.Picking (default true) controls capture. Captures can lag the latest rendered frame.
  xiiResult PickIcon(const xiiView& view, xiiUInt32 x, xiiUInt32 y, PickingResult& out_hit) const;
  /// Window pixels, exclusive right/bottom edges. Unique objects in first visible pixel order.
  xiiResult PickIconsInRect(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<PickingResult>& out_hits) const;

  float m_fSize          = 1.0f;
  float m_fMaxScreenSize = 64.0f;

private:
  xiiResult Collect(const xiiView& view);

  xiiDynamicArray<Icon>                  m_Icons;
  xiiUniquePtr<xiiSceneShapeIconPicking> m_pPicking;
};

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

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

  /// Inserts into the native default pipeline before Debug, retaining all scene shading stages.
  /// Does not access GAL; the returned definition is editable/serializable like any pipeline asset.
  static xiiResult                       GetPipelineDescriptor(xiiRenderPipelineResourceDescriptor& out_descriptor);
  static xiiRenderPipelineResourceHandle GetPipeline();

  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const override;
  xiiResult Build(xiiRenderPipelinePassContext& context) override;

  /// Borrowed until the next Build. Far-to-near order, with object identity breaking depth ties.
  xiiArrayPtr<const Icon> GetIcons() const { return m_Icons; }

  float m_fSize          = 1.0f;
  float m_fMaxScreenSize = 64.0f;

private:
  xiiResult Collect(const xiiView& view);

  xiiDynamicArray<Icon> m_Icons;
};

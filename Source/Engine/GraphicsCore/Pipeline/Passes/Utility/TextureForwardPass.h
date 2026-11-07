/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/HybridArray.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// An optional interface texture connection without GPU work or ownership.
class XII_GRAPHICSCORE_DLL xiiTextureForwardPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureForwardPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Input{xiiGALResourceFormat::Unknown, true};
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::Unknown, true, {}, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Read};
  /// Additional viewport modes that can activate an otherwise disabled connection.
  xiiHybridArray<xiiString, 4> m_EnabledViewModes;
  bool                         m_bEnabled      = true;
  bool                         m_bRequireInput = false;
  xiiResult                    Configure(const xiiVariantDictionary& properties) override;
  void                         GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const override;
  xiiResult                    Build(xiiRenderPipelinePassContext& context) override;

private:
  xiiHybridArray<xiiEnum<xiiViewRenderMode>, 4> m_ResolvedViewModes;
};

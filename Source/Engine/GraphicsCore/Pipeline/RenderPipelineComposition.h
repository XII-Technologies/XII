/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

/// Authoring node for an independently authored reusable pipeline. The asset compiler replaces
/// this node by a Core definition reference. It never becomes an executable runtime pass.
class XII_GRAPHICSCORE_DLL xiiRenderPipelineSubgraphPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineSubgraphPass, xiiRenderPipelinePass);

public:
  xiiString                                 m_sResource;
  xiiVariantDictionary                      m_Parameters;
  xiiDynamicArray<xiiGraphPropertyOverride> m_Overrides;
  void                                      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const override;
  xiiResult                                 Build(xiiRenderPipelinePassContext&) override { return XII_FAILURE; }
};

/// Authoring interface node. External inputs have an output pin inside the document; external
/// outputs have an input pin. Ordinary editor connections define the internal boundary mappings.
class XII_GRAPHICSCORE_DLL xiiRenderPipelineInterfacePass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineInterfacePass, xiiRenderPipelinePass);

public:
  xiiString                     m_sName          = "Input";
  bool                          m_bInput         = true;
  xiiUInt32                     m_uiResourceType = xiiRenderPipelinePin::Texture;
  xiiEnum<xiiGALResourceFormat> m_Format         = xiiGALResourceFormat::Unknown;
  xiiString                     m_sDataType;
  void                          GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const override;
  xiiResult                     Build(xiiRenderPipelinePassContext&) override { return XII_FAILURE; }
};

/// Infer the external schema from configured reflected internal pass pins, including nested
/// mappings and overrides. Asset-authored schemas cannot override native resource types.
XII_GRAPHICSCORE_DLL xiiResult xiiGetRenderPipelineInterface(const xiiRenderPipelineResourceDescriptor& descriptor, xiiDynamicArray<xiiRenderPipelinePin>& out_pins, xiiStringBuilder* out_error = nullptr);

/// Apply authored instance-interface requirements through Core's occurrence boundary map.
/// Used by compilation and editor interface inference so both see identical contracts.
XII_GRAPHICSCORE_DLL xiiResult xiiApplyRenderPipelineInterfaceContracts(const xiiGraphDefinition& root, xiiGraphComposition::Resolver resolver,
                                                                        const xiiGraphExpansion& expansion, xiiArrayPtr<xiiArrayPtr<xiiRenderPipelinePin>> pins, xiiStringBuilder* out_error = nullptr);

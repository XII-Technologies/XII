/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Variant.h>
#include <GraphicsCore/Pipeline/RenderExtraction.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderPipelinePin.h>
#include <GraphicsCore/Pipeline/RenderPipelineSettings.h>

class xiiView;
struct xiiRenderPipelinePassContext;
struct xiiRenderPipelineResourceDescriptor;

struct xiiRenderPipelineBinding
{
  xiiRenderGraphTextureHandle m_hTexture;
  xiiRenderGraphBufferHandle  m_hBuffer;
};

/// One mutable instance per pass per view. Constructors/Configure/GetPins must not access GAL.
/// Build emits frame-specific work through the specialized render graph. Execute delegates must
/// capture owned instance state or frame data, never this stack context. Instances remain alive until
/// their graph execution has finished. Persistent GAL objects follow the device's deferred lifetime rules.
class XII_GRAPHICSCORE_DLL xiiRenderPipelinePass : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelinePass, xiiReflectedClass);

public:
  virtual ~xiiRenderPipelinePass();
  /// Authored, occurrence-local parameter overrides. Kept separate from compile-time pass properties.
  xiiVariantDictionary m_ParameterOverrides;
  /// Authoring-only owned objects. Runtime pass instances consume compiled pins instead.
  xiiDynamicArray<xiiRenderPipelinePinConfiguration*> m_PinConfigurations;
  virtual xiiResult                                   Configure(const xiiVariantDictionary& properties);
  /// Capture authored reflected properties, including arrays, value maps and owned settings objects.
  /// Occurrence-local ParameterOverrides remain in the descriptor's parameter scope.
  xiiResult GetProperties(xiiVariantDictionary& out_properties) const;
  /// Static pins are discovered from typed XII_MEMBER_PROPERTY members. Only
  /// authoring interfaces whose topology comes from another definition are dynamic.
  virtual void                                        GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const;
  xiiResult                                           InitializePinBindings(xiiArrayPtr<const xiiRenderPipelinePin> pins) const;
  xiiResult                                           GetConfiguredPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins, xiiStringBuilder* out_error = nullptr) const;
  virtual xiiBitflags<xiiRenderPipelineDeviceFeature> GetRequiredDeviceFeatures() const { return {}; }
  virtual xiiBitflags<xiiRenderExtractionCategory>    GetExtractionRequirements() const { return {}; }
  virtual xiiBitflags<xiiRenderPipelineFeature>       GetRuntimeFeatures() const { return {}; }
  virtual xiiResult                                   Build(xiiRenderPipelinePassContext& context) = 0;
  /// Publish only previous-frame temporal outputs, without reading current-frame inputs.
  /// Optional history may be absent on the first frame. Promote it only after successful
  /// execution, and discard it in InvalidateTemporalHistory after a failed frame or camera cut.
  virtual xiiResult BuildTemporalHistory(xiiRenderPipelinePassContext&) { return XII_SUCCESS; }
  /// Optional domain validation after generic connectivity and ordering have been validated.
  /// Runs on CPU during compilation; failure prevents publication of the entire resource.
  virtual xiiResult ValidatePipeline(const xiiRenderPipelineResourceDescriptor&, xiiArrayPtr<const xiiUInt32>, xiiUInt32, xiiStringBuilder*) const { return XII_SUCCESS; }
  /// Clears view-local history after camera cuts and configuration changes.
  virtual void InvalidateTemporalHistory() {}

  xiiRenderPipelineOrderInputPin  m_Before;
  xiiRenderPipelineOrderOutputPin m_After;
};

/// Bindings use the pass schema's flat pin indices. Input bindings are resolved before Build;
/// output bindings are written by the pass. Input/output access does not allocate.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePassContext
{
  xiiView*                                 m_pView;
  xiiRenderGraph&                          m_Graph;
  xiiRenderGraphBlackboard&                m_Blackboard;
  const xiiVariantDictionary&              m_Parameters;
  xiiArrayPtr<const xiiRenderPipelinePin>  m_Pins;
  xiiArrayPtr<xiiRenderPipelineBinding>    m_Bindings;
  xiiStringView                            m_sPassName;
  xiiStringView                            m_sInstanceScope;
  xiiBitflags<xiiRenderExtractionCategory> m_ExtractionRequirements;
  xiiBitflags<xiiRenderPipelineFeature>    m_RuntimeFeatures;
  bool                                     m_bTemporalHistoryPhase = false;

  xiiUInt32                   FindPin(xiiStringView name, bool input) const;
  xiiRenderGraphTextureHandle GetInputTexture(xiiStringView name) const;
  xiiRenderGraphBufferHandle  GetInputBuffer(xiiStringView name) const;
  xiiResult                   SetOutput(xiiStringView name, xiiRenderGraphTextureHandle texture);
  xiiResult                   SetOutput(xiiStringView name, xiiRenderGraphBufferHandle buffer);
  xiiRenderGraphTextureHandle GetInputTexture(const xiiRenderPipelineTextureInputPin& pin) const;
  xiiRenderGraphBufferHandle  GetInputBuffer(const xiiRenderPipelineBufferInputPin& pin) const;
  xiiResult                   SetOutput(const xiiRenderPipelineTextureOutputPin& pin, xiiRenderGraphTextureHandle texture);
  xiiResult                   SetOutput(const xiiRenderPipelineBufferOutputPin& pin, xiiRenderGraphBufferHandle buffer);
};

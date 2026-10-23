/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Math/Size.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Variant.h>
#include <GraphicsCore/Pipeline/RenderExtraction.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderPipelineFrameData.h>
#include <GraphicsCore/Pipeline/RenderPipelinePin.h>
#include <GraphicsCore/Pipeline/RenderPipelineSettings.h>
#include <GraphicsFoundation/Resources/TopLevelAS.h>

class xiiView;
struct xiiRenderPipelinePassContext;
struct xiiRenderPipelineResourceDescriptor;

/// Frame-local TLAS ownership and its graph version establishing build readiness.
/// Consumers must read m_Dependency in BuildASRead state before tracing.
struct xiiRenderPipelineAccelerationStructureBinding
{
  xiiSharedPtr<xiiGALTopLevelAS> m_pStructure;
  xiiRenderGraphBufferHandle     m_Dependency;
  bool                           IsValid() const { return m_pStructure != nullptr && m_Dependency.IsValid(); }
  bool                           IsEmpty() const { return m_pStructure == nullptr && !m_Dependency.IsValid(); }
};

struct xiiRenderPipelineBinding
{
  xiiRenderGraphTextureHandle                    m_hTexture;
  xiiRenderGraphBufferHandle                     m_hBuffer;
  xiiRenderPipelineAccelerationStructureBinding  m_AccelerationStructure;
  xiiSharedPtr<const xiiRenderPipelineFrameData> m_pFrameData;
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
  virtual void GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const;
  xiiResult    InitializePinBindings(xiiArrayPtr<const xiiRenderPipelinePin> pins) const;
  xiiResult    GetConfiguredPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins, xiiStringBuilder* out_error = nullptr) const;
  /// Apply the same authored contracts to declarations resolved by an external authoring context.
  xiiResult                                           GetConfiguredPins(xiiArrayPtr<const xiiRenderPipelinePin> declarations, xiiDynamicArray<xiiRenderPipelinePin>& out_pins, xiiStringBuilder* out_error = nullptr) const;
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
  /// Build-only diagnostic sink; execute delegates must not retain this context.
  xiiStringBuilder* m_pBuildError = nullptr;
  xiiResult         ReportFailure(xiiStringView reason) const;

  /// Resolve a concrete frame-local extent without changing the compiled pin or an input texture.
  xiiResult ResolveTextureSize(const xiiRenderPipelinePin& pin, xiiRenderGraphTextureHandle reference, xiiSizeU32& out_size) const;
  xiiResult ResolveTextureSize(const xiiRenderPipelineTextureOutputPin& pin, xiiRenderGraphTextureHandle reference, xiiSizeU32& out_size) const;
  xiiResult ValidateTexture(const xiiRenderPipelinePin& pin, xiiRenderGraphTextureHandle texture) const;

  xiiUInt32                                      FindPin(xiiStringView name, bool input) const;
  xiiRenderGraphTextureHandle                    GetInputTexture(xiiStringView name) const;
  xiiRenderGraphBufferHandle                     GetInputBuffer(xiiStringView name) const;
  xiiResult                                      SetOutput(xiiStringView name, xiiRenderGraphTextureHandle texture);
  xiiResult                                      SetOutput(xiiStringView name, xiiRenderGraphBufferHandle buffer);
  xiiRenderGraphTextureHandle                    GetInputTexture(const xiiRenderPipelineTextureInputPin& pin) const;
  xiiRenderGraphBufferHandle                     GetInputBuffer(const xiiRenderPipelineBufferInputPin& pin) const;
  xiiResult                                      SetOutput(const xiiRenderPipelineTextureOutputPin& pin, xiiRenderGraphTextureHandle texture);
  xiiResult                                      SetOutput(const xiiRenderPipelineBufferOutputPin& pin, xiiRenderGraphBufferHandle buffer);
  xiiRenderPipelineAccelerationStructureBinding  GetInputAccelerationStructure(const xiiRenderPipelineAccelerationStructureInputPin& pin) const;
  xiiResult                                      SetOutput(const xiiRenderPipelineAccelerationStructureOutputPin& pin, const xiiRenderPipelineAccelerationStructureBinding& structure);
  xiiSharedPtr<const xiiRenderPipelineFrameData> GetInputData(const xiiRenderPipelineDataInputPin& pin) const;
  xiiResult                                      SetOutput(const xiiRenderPipelineDataOutputPin& pin, xiiSharedPtr<const xiiRenderPipelineFrameData> data);
};

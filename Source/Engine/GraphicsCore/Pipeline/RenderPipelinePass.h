/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Variant.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

class xiiView;
struct xiiRenderPipelinePassContext;

/// Pin schemas are supplied by the reflected pass type, rather than trusted asset data.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePin
{
  enum Type : xiiUInt8
  {
    Texture,
    Buffer,
    Order
  };
  xiiString m_sName;
  Type      m_Type      = Texture;
  bool      m_bInput    = true;
  bool      m_bOptional = false;
  /// Unknown accepts the producer's format; concrete formats must match.
  xiiGALResourceFormat::Enum m_Format = xiiGALResourceFormat::Unknown;
};

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
  virtual ~xiiRenderPipelinePass() = default;
  virtual xiiResult Configure(const xiiVariantDictionary& properties);
  virtual void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const = 0;
  virtual xiiResult Build(xiiRenderPipelinePassContext& context)                   = 0;
};

/// Bindings use the pass schema's flat pin indices. Input bindings are resolved before Build;
/// output bindings are written by the pass. Input/output access does not allocate.
struct XII_GRAPHICSCORE_DLL xiiRenderPipelinePassContext
{
  xiiView*                                m_pView;
  xiiRenderGraph&                         m_Graph;
  xiiRenderGraphBlackboard&               m_Blackboard;
  const xiiVariantDictionary&             m_Parameters;
  xiiArrayPtr<const xiiRenderPipelinePin> m_Pins;
  xiiArrayPtr<xiiRenderPipelineBinding>   m_Bindings;
  xiiStringView                           m_sPassName;

  xiiUInt32                   FindPin(xiiStringView name, bool input) const;
  xiiRenderGraphTextureHandle GetInputTexture(xiiStringView name) const;
  xiiRenderGraphBufferHandle  GetInputBuffer(xiiStringView name) const;
  xiiResult                   SetOutput(xiiStringView name, xiiRenderGraphTextureHandle texture);
  xiiResult                   SetOutput(xiiStringView name, xiiRenderGraphBufferHandle buffer);
};

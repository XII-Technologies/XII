#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

#include <GraphicsCore/Pipeline/Declarations.h>

class xiiRenderPipelineNode;

struct xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  struct Type
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      Input           = XII_BIT(0), ///< This pin receives data, used to consume textures or resources.
      Output          = XII_BIT(1), ///< This pin produces data, used to declare texture or buffer outputs.
      PassThrough     = XII_BIT(2), ///< A special connection that routes data through without modifying it.
      TextureProvider = XII_BIT(3), ///< Marks that this pin provides a texture dynamically each frame.

      Default = 0U
    };

    struct Bits
    {
      StorageType Input : 1;
      StorageType Output : 1;
      StorageType PassThrough : 1;
      StorageType TextureProvider : 1;
    };
  };

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;

  xiiResult Deserialize(xiiStreamReader& inout_stream);

  xiiBitflags<Type>      m_Type;
  xiiUInt8               m_uiInputIndex  = 0xFFU;
  xiiUInt8               m_uiOutputIndex = 0xFFU;
  xiiRenderPipelineNode* m_pParent       = nullptr;

  xiiEnum<xiiSourceFormat>                m_Format                          = xiiSourceFormat::Default;
  xiiEnum<xiiGALMSAASampleCount>          m_SampleCount                     = xiiGALMSAASampleCount::OneSample;
  xiiEnum<xiiGALAttachmentLoadOperation>  m_AttachmentLoadOperation         = xiiGALAttachmentLoadOperation::Load;
  xiiEnum<xiiGALAttachmentStoreOperation> m_AttachmentStoreOperation        = xiiGALAttachmentStoreOperation::Store;
  xiiEnum<xiiGALAttachmentLoadOperation>  m_AttachmentStencilLoadOperation  = xiiGALAttachmentLoadOperation::Load;
  xiiEnum<xiiGALAttachmentStoreOperation> m_AttachmentStencilStoreOperation = xiiGALAttachmentStoreOperation::Store;
  xiiColor                                m_ClearColor                      = xiiColor::Black;
  float                                   m_fDepthClearValue                = 1.0f;
  xiiUInt8                                m_uiStencilClearValue             = 0U;
};

XII_DECLARE_FLAGS_OPERATORS(xiiRenderPipelineNodePin::Type);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePin);

struct xiiRenderPipelineNodeInputPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputPin() { m_Type = Type::Input; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputPin);

struct xiiRenderPipelineNodeOutputPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputPin() { m_Type = Type::Output; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputPin);

struct xiiRenderPipelineNodeInputProviderPin : public xiiRenderPipelineNodeInputPin
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputProviderPin() { m_Type = Type::Input | Type::TextureProvider; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputProviderPin);

struct xiiRenderPipelineNodeOutputProviderPin : public xiiRenderPipelineNodeOutputPin
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputProviderPin() { m_Type = Type::Output | Type::TextureProvider; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputProviderPin);

struct xiiRenderPipelineNodePassThroughPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiRenderPipelineNodePassThroughPin() { m_Type = Type::PassThrough; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePassThroughPin);

class XII_GRAPHICSCORE_DLL xiiRenderPipelineNode : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineNode, xiiReflectedClass);

public:
  virtual ~xiiRenderPipelineNode() = default;

  void InitializePins();

  xiiHashedString                 GetPinName(const xiiRenderPipelineNodePin* pPin) const;
  const xiiRenderPipelineNodePin* GetPinByName(xiiStringView sName) const;
  const xiiRenderPipelineNodePin* GetPinByName(xiiHashedString sName) const;

  XII_ALWAYS_INLINE const xiiArrayPtr<const xiiRenderPipelineNodePin* const> GetInputPins() const { return m_InputPins; }
  XII_ALWAYS_INLINE const xiiArrayPtr<const xiiRenderPipelineNodePin* const> GetOutputPins() const { return m_OutputPins; }

private:
  xiiDynamicArray<const xiiRenderPipelineNodePin*>               m_InputPins;
  xiiDynamicArray<const xiiRenderPipelineNodePin*>               m_OutputPins;
  xiiHashTable<xiiHashedString, const xiiRenderPipelineNodePin*> m_NameToPin;
};

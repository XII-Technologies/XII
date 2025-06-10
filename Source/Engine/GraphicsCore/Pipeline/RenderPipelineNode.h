#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiRenderPipelineNode;

struct xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  struct Type
  {
    using StorageType = xiiUInt8;

    enum Enum : StorageType
    {
      Input           = XII_BIT(0),
      Output          = XII_BIT(1),
      PassThrough     = XII_BIT(2),
      TextureProvider = XII_BIT(3), ///< Pass provides pin texture to the pipeline each frame.

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

  xiiBitflags<Type>      m_Type;
  xiiUInt8               m_uiInputIndex  = 0xFFU;
  xiiUInt8               m_uiOutputIndex = 0xFFU;
  xiiRenderPipelineNode* m_pParent       = nullptr;
};
XII_DECLARE_FLAGS_OPERATORS(xiiRenderPipelineNodePin::Type);

struct xiiRenderPipelineNodeInputPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputPin() { m_Type = Type::Input; }
};

struct xiiRenderPipelineNodeOutputPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputPin() { m_Type = Type::Output; }
};

struct xiiRenderPipelineNodeInputProviderPin : public xiiRenderPipelineNodeInputPin
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiRenderPipelineNodeInputProviderPin() { m_Type = Type::Input | Type::TextureProvider; }
};

struct xiiRenderPipelineNodeOutputProviderPin : public xiiRenderPipelineNodeOutputPin
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiRenderPipelineNodeOutputProviderPin() { m_Type = Type::Output | Type::TextureProvider; }
};

struct xiiRenderPipelineNodePassThroughPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiRenderPipelineNodePassThroughPin() { m_Type = Type::PassThrough; }
};

class XII_GRAPHICSCORE_DLL xiiRenderPipelineNode : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineNode, xiiReflectedClass);

public:
  virtual ~xiiRenderPipelineNode() = default;

  void InitializePins();

  xiiHashedString                 GetPinName(const xiiRenderPipelineNodePin* pPin) const;
  const xiiRenderPipelineNodePin* GetPinByName(xiiStringView sName) const;
  const xiiRenderPipelineNodePin* GetPinByName(xiiHashedString sName) const;

  const xiiArrayPtr<const xiiRenderPipelineNodePin* const> GetInputPins() const { return m_InputPins; }
  const xiiArrayPtr<const xiiRenderPipelineNodePin* const> GetOutputPins() const { return m_OutputPins; }

private:
  xiiDynamicArray<const xiiRenderPipelineNodePin*>               m_InputPins;
  xiiDynamicArray<const xiiRenderPipelineNodePin*>               m_OutputPins;
  xiiHashTable<xiiHashedString, const xiiRenderPipelineNodePin*> m_NameToPin;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePin);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputPin);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputPin);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeInputProviderPin);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodeOutputProviderPin);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePassThroughPin);

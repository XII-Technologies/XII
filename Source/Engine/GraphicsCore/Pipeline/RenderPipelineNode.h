#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsCore/Declarations.h>

class xiiRenderPipelineNode;

struct xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  struct Type
  {
    using StorageType = xiiUInt8;

    enum Enum
    {
      Unknown,
      Input,
      Output,
      PassThrough,

      Default = Unknown
    };
  };

  xiiEnum<Type>          m_Type;
  xiiUInt8               m_uiInputIndex  = 0xFF;
  xiiUInt8               m_uiOutputIndex = 0xFF;
  xiiRenderPipelineNode* m_pParent       = nullptr;
};

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

struct xiiRenderPipelineNodePassThrougPin : public xiiRenderPipelineNodePin
{
  XII_DECLARE_POD_TYPE();

  XII_ALWAYS_INLINE xiiRenderPipelineNodePassThrougPin() { m_Type = Type::PassThrough; }
};

class XII_GRAPHICSCORE_DLL xiiRenderPipelineNode : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineNode, xiiReflectedClass);

public:
  virtual ~xiiRenderPipelineNode() = default;

  void InitializePins();

  xiiHashedString                                          GetPinName(const xiiRenderPipelineNodePin* pPin) const;
  const xiiRenderPipelineNodePin*                          GetPinByName(const char* szName) const;
  const xiiRenderPipelineNodePin*                          GetPinByName(xiiHashedString sName) const;
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
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineNodePassThrougPin);

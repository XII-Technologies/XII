#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/GameEngineDLL.h>

class xiiVisualScriptNode;

using xiiVisualScriptResourceHandle = xiiTypedResourceHandle<class xiiVisualScriptResource>;

/// \brief Describes a visual script graph (node types and connections)
struct XII_GAMEENGINE_DLL xiiVisualScriptResourceDescriptor
{
  void Load(xiiStreamReader& stream);
  void Save(xiiStreamWriter& stream) const;
  void PrecomputeMessageHandlers();

  struct Node
  {
    Node()
    {
      m_isMsgSender    = 0;
      m_isMsgHandler   = 0;
      m_isFunctionCall = 0;
    }

    xiiString      m_sTypeName;       ///< This is what gets written to the file if m_pType is null.
    const xiiRTTI* m_pType = nullptr; ///< Cached resolved type pointer after loading

    xiiUInt16 m_uiFirstProperty = 0;
    xiiUInt8  m_uiNumProperties = 0;
    xiiUInt8  m_isMsgSender : 1;
    xiiUInt8  m_isMsgHandler : 1;
    xiiUInt8  m_isFunctionCall : 1;
  };

  struct ExecutionConnection
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt16 m_uiSourceNode;
    xiiUInt16 m_uiTargetNode;
    xiiUInt8  m_uiOutputPin;
    xiiUInt8  m_uiInputPin;
  };

  struct DataConnection
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt16 m_uiSourceNode;
    xiiUInt16 m_uiTargetNode;
    xiiUInt8  m_uiOutputPin;
    xiiUInt8  m_uiOutputPinType; // xiiVisualScriptDataPinType
    xiiUInt8  m_uiInputPin;
    xiiUInt8  m_uiInputPinType; // xiiVisualScriptDataPinType
  };

  struct Property
  {
    xiiString  m_sName; // name of the property, as shown in the UI (unless translated)
    xiiVariant m_Value;
    xiiInt32   m_iMappingIndex = xiiMath::MaxValue<xiiInt32>(); // can (optionally) be used to map the property to something
  };

  struct LocalParameter
  {
    xiiHashedString m_sName;
  };

  struct LocalParameterBool : LocalParameter
  {
    bool m_Value = false;
  };

  struct LocalParameterNumber : LocalParameter
  {
    double m_Value = 0;
  };

  struct LocalParameterString : LocalParameter
  {
    xiiString m_sValue;
  };

  void AssignNodeProperties(xiiVisualScriptNode& vsNode, const Node& properties) const;

  xiiDynamicArray<Node>                 m_Nodes;
  xiiDynamicArray<ExecutionConnection>  m_ExecutionPaths;
  xiiDynamicArray<DataConnection>       m_DataPaths;
  xiiArrayMap<xiiMessageId, xiiUInt16>  m_MessageHandlers;
  xiiDeque<Property>                    m_Properties;
  xiiDynamicArray<LocalParameterBool>   m_BoolParameters;
  xiiDynamicArray<LocalParameterNumber> m_NumberParameters;
  xiiDynamicArray<LocalParameterString> m_StringParameters;
};

class XII_GAMEENGINE_DLL xiiVisualScriptResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiVisualScriptResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiVisualScriptResource, xiiVisualScriptResourceDescriptor);

public:
  xiiVisualScriptResource();
  ~xiiVisualScriptResource();

  const xiiVisualScriptResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiVisualScriptResourceDescriptor m_Descriptor;
};

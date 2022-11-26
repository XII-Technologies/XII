#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/String.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class xiiAbstractFunctionProperty;

struct xiiVisualScriptPinDescriptor
{
  enum class PinType : xiiUInt8
  {
    Execution,
    Data
  };

  xiiString                           m_sName;
  xiiString                           m_sTooltip;
  xiiColorGammaUB                     m_Color;
  xiiEnum<xiiVisualScriptDataPinType> m_DataType;
  PinType                             m_PinType;
  xiiUInt8                            m_uiPinIndex;
};

struct xiiVisualScriptNodeDescriptor
{
  xiiString       m_sTypeName;
  xiiString       m_sCategory;
  xiiString       m_sTitle;
  xiiColorGammaUB m_Color;

  xiiHybridArray<xiiVisualScriptPinDescriptor, 4>   m_InputPins;
  xiiHybridArray<xiiVisualScriptPinDescriptor, 4>   m_OutputPins;
  xiiHybridArray<xiiReflectedPropertyDescriptor, 4> m_Properties;
};

class xiiVisualScriptTypeRegistry
{
  XII_DECLARE_SINGLETON(xiiVisualScriptTypeRegistry);

public:
  xiiVisualScriptTypeRegistry();
  ~xiiVisualScriptTypeRegistry();
  const xiiVisualScriptNodeDescriptor* GetDescriptorForType(const xiiRTTI* pRtti) const;

  const xiiRTTI* GetNodeBaseType() const { return m_pBaseType; }

  void UpdateNodeTypes();

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, VisualScript);

  void           PhantomTypeRegistryEventHandler(const xiiPhantomRttiManagerEvent& e);
  void           UpdateNodeType(const xiiRTTI* pRtti);
  const xiiRTTI* GenerateTypeFromDesc(const xiiVisualScriptNodeDescriptor& desc);
  void           CreateMessageSenderNodeType(const xiiRTTI* pRtti);
  void           CreateMessageHandlerNodeType(const xiiRTTI* pRtti);
  void           CreateFunctionCallNodeType(const xiiRTTI* pRtti, const xiiAbstractFunctionProperty* pFunction);

  xiiMap<const xiiRTTI*, xiiVisualScriptNodeDescriptor> m_NodeDescriptors;

  const xiiRTTI* m_pBaseType;
};

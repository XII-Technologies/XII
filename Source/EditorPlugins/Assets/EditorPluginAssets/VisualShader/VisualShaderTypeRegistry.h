#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class xiiOpenDdlReaderElement;

struct xiiVisualShaderPinDescriptor
{
  xiiString                      m_sName;
  const xiiRTTI*                 m_pDataType = nullptr;
  xiiReflectedPropertyDescriptor m_PropertyDesc;
  xiiColorGammaUB                m_Color             = xiiColorScheme::DarkUI(xiiColorScheme::Gray);
  bool                           m_bExposeAsProperty = false;
  xiiString                      m_sDefaultValue;
  xiiDynamicArray<xiiString>     m_sDefinesWhenUsingDefaultValue;
  xiiString                      m_sShaderCodeInline;
  xiiString                      m_sTooltip;
};

struct xiiVisualShaderNodeType
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Generic,
    Main,
    Texture,

    Default = Generic
  };
};

struct xiiVisualShaderNodeDescriptor
{
  xiiEnum<xiiVisualShaderNodeType> m_NodeType;
  xiiString                        m_sCfgFile; ///< from which config file this node type was loaded
  xiiString                        m_sName;
  xiiHashedString                  m_sCategory;
  xiiString                        m_sCheckPermutations;
  xiiColorGammaUB                  m_Color = xiiColorScheme::DarkUI(xiiColorScheme::Gray);
  xiiString                        m_sShaderCodePixelDefines;
  xiiString                        m_sShaderCodePixelIncludes;
  xiiString                        m_sShaderCodePixelSamplers;
  xiiString                        m_sShaderCodePixelConstants;
  xiiString                        m_sShaderCodePixelBody;
  xiiString                        m_sShaderCodePermutations;
  xiiString                        m_sShaderCodeMaterialParams;
  xiiString                        m_sShaderCodeMaterialCB;
  xiiString                        m_sShaderCodeRenderState;
  xiiString                        m_sShaderCodeVertexShader;
  xiiString                        m_sShaderCodeGeometryShader;

  xiiHybridArray<xiiVisualShaderPinDescriptor, 4>   m_InputPins;
  xiiHybridArray<xiiVisualShaderPinDescriptor, 4>   m_OutputPins;
  xiiHybridArray<xiiReflectedPropertyDescriptor, 4> m_Properties;
  xiiHybridArray<xiiInt8, 4>                        m_UniquePropertyValueGroups; // no property in the same group may share the same value, -1 for disabled
};


class xiiVisualShaderTypeRegistry
{
  XII_DECLARE_SINGLETON(xiiVisualShaderTypeRegistry);

public:
  xiiVisualShaderTypeRegistry();

  const xiiVisualShaderNodeDescriptor* GetDescriptorForType(const xiiRTTI* pRtti) const;

  const xiiRTTI* GetNodeBaseType() const { return m_pBaseType; }

  const xiiRTTI* GetPinSamplerType() const { return m_pSamplerPinType; }

  void UpdateNodeData();

  void UpdateNodeData(xiiStringView sCfgFileRelative);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorPluginAssets, VisualShader);

  void           LoadNodeData();
  const xiiRTTI* GenerateTypeFromDesc(const xiiVisualShaderNodeDescriptor& desc);
  void           LoadConfigFile(const char* szFile);

  void ExtractNodePins(const xiiOpenDdlReaderElement* pNode, const char* szPinType, xiiHybridArray<xiiVisualShaderPinDescriptor, 4>& pinArray, bool bOutput);
  void ExtractNodeProperties(const xiiOpenDdlReaderElement* pNode, xiiVisualShaderNodeDescriptor& nd);
  void ExtractNodeConfig(const xiiOpenDdlReaderElement* pNode, xiiVisualShaderNodeDescriptor& nd);


  xiiMap<const xiiRTTI*, xiiVisualShaderNodeDescriptor> m_NodeDescriptors;

  const xiiRTTI* m_pBaseType;
  const xiiRTTI* m_pSamplerPinType;
};

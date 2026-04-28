/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

XII_IMPLEMENT_SINGLETON(xiiVisualShaderTypeRegistry);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, VisualShader)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiVisualShaderTypeRegistry);

    xiiVisualShaderTypeRegistry::GetSingleton()->LoadNodeData();
    const xiiRTTI* pBaseType = xiiVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

    xiiQtNodeScene::GetPinFactory().RegisterCreator(xiiGetStaticRTTI<xiiVisualShaderPin>(), [](const xiiRTTI* pRtti)->xiiQtPin* { return new xiiQtVisualShaderPin(); });
    xiiQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const xiiRTTI* pRtti)->xiiQtNode* { return new xiiQtVisualShaderNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const xiiRTTI* pBaseType = xiiVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

    xiiQtNodeScene::GetPinFactory().UnregisterCreator(xiiGetStaticRTTI<xiiVisualShaderPin>());
    xiiQtNodeScene::GetNodeFactory().UnregisterCreator(pBaseType);

    xiiVisualShaderTypeRegistry* pDummy = xiiVisualShaderTypeRegistry::GetSingleton();
    XII_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  static const char* s_szColorNames[] = {
    "Red",
    "Pink",
    "Grape",
    "Violet",
    "Indigo",
    "Blue",
    "Cyan",
    "Teal",
    "Green",
    "Lime",
    "Yellow",
    "Orange",
    "Gray",
  };
  static_assert(XII_ARRAY_SIZE(s_szColorNames) == xiiColorScheme::Count);

  static void GetColorFromDdl(const xiiOpenDdlReaderElement* pElement, xiiColorGammaUB& out_color)
  {
    if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::String)
    {
      xiiColorScheme::Enum color  = xiiColorScheme::Gray;
      const xiiStringView* pValue = pElement->GetPrimitivesString();
      for (xiiUInt32 i = 0; i < xiiColorScheme::Count; ++i)
      {
        if (pValue->IsEqual_NoCase(s_szColorNames[i]))
        {
          color = static_cast<xiiColorScheme::Enum>(i);
          break;
        }
      }

      out_color = xiiColorScheme::DarkUI(color);
    }
    else
    {
      xiiOpenDdlUtils::ConvertToColorGamma(pElement, out_color).IgnoreResult();
    }
  }
} // namespace

xiiVisualShaderTypeRegistry::xiiVisualShaderTypeRegistry() :
  m_SingletonRegistrar(this), m_pBaseType(nullptr), m_pSamplerPinType(nullptr)
{
}

const xiiVisualShaderNodeDescriptor* xiiVisualShaderTypeRegistry::GetDescriptorForType(const xiiRTTI* pRtti) const
{
  auto it = m_NodeDescriptors.Find(pRtti);

  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}

void xiiVisualShaderTypeRegistry::UpdateNodeData()
{
  xiiStringBuilder sSearchDir = xiiApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSearchDir.AppendPath("VisualShader/*.ddl");

  xiiFileSystemIterator it;
  for (it.StartSearch(sSearchDir, xiiFileSystemIteratorFlags::ReportFiles); it.IsValid(); it.Next())
  {
    UpdateNodeData(it.GetStats().m_sName);
  }
}

void xiiVisualShaderTypeRegistry::UpdateNodeData(xiiStringView sCfgFileRelative)
{
  xiiStringBuilder sPath = sCfgFileRelative;
  if (!xiiPathUtils::IsAbsolutePath(sCfgFileRelative))
  {
    sPath.SetFormat(":app/VisualShader/{}", sCfgFileRelative);
  }
  LoadConfigFile(sPath);
}

void xiiVisualShaderTypeRegistry::LoadNodeData()
{
  // Base Node Type
  if (m_pBaseType == nullptr)
  {
    xiiReflectedTypeDescriptor desc;
    desc.m_sTypeName       = "xiiVisualShaderNodeBase";
    desc.m_sPluginName     = "VisualShaderTypes";
    desc.m_sParentTypeName = xiiGetStaticRTTI<xiiReflectedClass>()->GetTypeName();
    desc.m_Flags           = xiiTypeFlags::Phantom | xiiTypeFlags::Abstract | xiiTypeFlags::Class;
    desc.m_uiTypeVersion   = 1;

    m_pBaseType = xiiPhantomRttiManager::RegisterType(desc);
  }

  if (m_pSamplerPinType == nullptr)
  {
    xiiReflectedTypeDescriptor desc;
    desc.m_sTypeName       = "xiiVisualShaderSamplerPin";
    desc.m_sPluginName     = "VisualShaderTypes";
    desc.m_sParentTypeName = xiiGetStaticRTTI<xiiReflectedClass>()->GetTypeName();
    desc.m_Flags           = xiiTypeFlags::Phantom | xiiTypeFlags::Class;
    desc.m_uiTypeVersion   = 1;

    m_pSamplerPinType = xiiPhantomRttiManager::RegisterType(desc);
  }

  UpdateNodeData();
}

const xiiRTTI* xiiVisualShaderTypeRegistry::GenerateTypeFromDesc(const xiiVisualShaderNodeDescriptor& nd)
{
  xiiStringBuilder temp;
  temp.Set("ShaderNode::", nd.m_sName);

  xiiReflectedTypeDescriptor desc;
  desc.m_sTypeName       = temp;
  desc.m_sPluginName     = "VisualShaderTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags           = xiiTypeFlags::Phantom | xiiTypeFlags::Class;
  desc.m_uiTypeVersion   = 1;
  desc.m_Properties      = nd.m_Properties;

  for (const auto& pin : nd.m_InputPins)
  {
    if (pin.m_PropertyDesc.m_sName.IsEmpty())
      continue;

    desc.m_Properties.PushBack(pin.m_PropertyDesc);
  }

  for (const auto& pin : nd.m_OutputPins)
  {
    if (pin.m_PropertyDesc.m_sName.IsEmpty())
      continue;

    desc.m_Properties.PushBack(pin.m_PropertyDesc);
  }

  return xiiPhantomRttiManager::RegisterType(desc);
}

void xiiVisualShaderTypeRegistry::LoadConfigFile(const char* szFile)
{
  XII_LOG_BLOCK("Loading Visual Shader Config", szFile);

  xiiLog::Debug("Loading VSE node config '{0}'", szFile);

  xiiFileReader file;
  if (file.Open(szFile).Failed())
  {
    xiiLog::Error("Failed to open Visual Shader config file '{0}'", szFile);
    return;
  }

  if (xiiPathUtils::HasExtension(szFile, "ddl"))
  {
    xiiOpenDdlReader ddl;
    if (ddl.ParseDocument(file, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
    {
      xiiLog::Error("Failed to parse Visual Shader config file '{0}'", szFile);
      return;
    }

    const xiiOpenDdlReaderElement* pRoot = ddl.GetRootElement();
    const xiiOpenDdlReaderElement* pNode = pRoot->GetFirstChild();

    while (pNode != nullptr)
    {
      if (!pNode->IsCustomType() || pNode->GetCustomType() != "Node")
      {
        xiiLog::Error("Top-Level object is not a 'Node' type");
        continue;
      }

      xiiVisualShaderNodeDescriptor nd;
      nd.m_sCfgFile = szFile;
      nd.m_sName    = pNode->GetName();

      ExtractNodeConfig(pNode, nd);
      ExtractNodeProperties(pNode, nd);
      ExtractNodePins(pNode, "InputPin", nd.m_InputPins, false);
      ExtractNodePins(pNode, "OutputPin", nd.m_OutputPins, true);

      m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);

      pNode = pNode->GetSibling();
    }
  }
}

static xiiVariant ExtractDefaultValue(const xiiRTTI* pType, const char* szDefault)
{
  if (pType == xiiGetStaticRTTI<xiiString>())
  {
    return xiiVariant(szDefault);
  }

  if (pType == xiiGetStaticRTTI<bool>())
  {
    bool res = false;
    xiiConversionUtils::StringToBool(szDefault, res).IgnoreResult();
    return xiiVariant(res);
  }

  float values[4] = {0, 0, 0, 0};
  xiiConversionUtils::ExtractFloatsFromString(szDefault, 4, values);

  if (pType == xiiGetStaticRTTI<float>())
  {
    return xiiVariant(values[0]);
  }

  if (pType == xiiGetStaticRTTI<int>())
  {
    return xiiVariant((int)values[0]);
  }

  if (pType == xiiGetStaticRTTI<xiiVec2>())
  {
    return xiiVariant(xiiVec2(values[0], values[1]));
  }

  if (pType == xiiGetStaticRTTI<xiiVec3>())
  {
    return xiiVariant(xiiVec3(values[0], values[1], values[2]));
  }

  if (pType == xiiGetStaticRTTI<xiiVec4>())
  {
    return xiiVariant(xiiVec4(values[0], values[1], values[2], values[3]));
  }

  if (pType == xiiGetStaticRTTI<xiiColor>())
  {
    return xiiVariant(xiiColorGammaUB(values[0], values[1], values[2], values[3]));
  }

  return xiiVariant();
}

void xiiVisualShaderTypeRegistry::ExtractNodePins(const xiiOpenDdlReaderElement* pNode, const char* szPinType, xiiHybridArray<xiiVisualShaderPinDescriptor, 4>& pinArray, bool bOutput)
{
  for (const xiiOpenDdlReaderElement* pElement = pNode->GetFirstChild(); pElement != nullptr; pElement = pElement->GetSibling())
  {
    if (pElement->GetCustomType() == szPinType)
    {
      xiiVisualShaderPinDescriptor pin;

      if (!pElement->HasName())
      {
        xiiLog::Error("Missing or invalid name for pin");
        continue;
      }

      pin.m_sName = pElement->GetName();

      auto pType = pElement->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Type");

      if (!pType)
      {
        xiiLog::Error("Missing or invalid pin type");
        continue;
      }

      {
        const xiiString& sType = pType->GetPrimitivesString()[0];

        if (sType == "color")
          pin.m_pDataType = xiiGetStaticRTTI<xiiColor>();
        else if (sType == "float4")
          pin.m_pDataType = xiiGetStaticRTTI<xiiVec4>();
        else if (sType == "float3")
          pin.m_pDataType = xiiGetStaticRTTI<xiiVec3>();
        else if (sType == "float2")
          pin.m_pDataType = xiiGetStaticRTTI<xiiVec2>();
        else if (sType == "float")
          pin.m_pDataType = xiiGetStaticRTTI<float>();
        else if (sType == "string")
          pin.m_pDataType = xiiGetStaticRTTI<xiiString>();
        else if (sType == "sampler")
          pin.m_pDataType = m_pSamplerPinType;
        else
        {
          xiiLog::Error("Invalid pin type '{0}'", sType);
          continue;
        }
      }

      if (auto pInline = pElement->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Inline"))
      {
        pin.m_sShaderCodeInline = pInline->GetPrimitivesString()[0];
      }
      else if (bOutput)
      {
        xiiLog::Error("Output pin '{0}' has no inline code specified", pin.m_sName);
        continue;
      }

      // this is optional
      if (auto pColor = pElement->FindChild("Color"))
      {
        GetColorFromDdl(pColor, pin.m_Color);
      }

      // this is optional
      if (auto pTooltip = pElement->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Tooltip"))
      {
        pin.m_sTooltip = pTooltip->GetPrimitivesString()[0];
      }

      // this is optional
      if (auto pDefaultValue = pElement->FindChildOfType(xiiOpenDdlPrimitiveType::String, "DefaultValue"))
      {
        pin.m_sDefaultValue = pDefaultValue->GetPrimitivesString()[0];
      }

      if (auto pDefineWhenUsingDefaultValue = pElement->FindChildOfType(xiiOpenDdlPrimitiveType::String, "DefineWhenUsingDefaultValue"))
      {
        const xiiUInt32 numElements = pDefineWhenUsingDefaultValue->GetNumPrimitives();
        pin.m_sDefinesWhenUsingDefaultValue.Reserve(numElements);

        for (xiiUInt32 i = 0; i < numElements; ++i)
        {
          pin.m_sDefinesWhenUsingDefaultValue.PushBack(pDefineWhenUsingDefaultValue->GetPrimitivesString()[i]);
        }
      }

      // this is optional
      if (auto pExpose = pElement->FindChildOfType(xiiOpenDdlPrimitiveType::Bool, "Expose"))
      {
        pin.m_bExposeAsProperty = pExpose->GetPrimitivesBool()[0];
      }

      if (pin.m_bExposeAsProperty)
      {
        pin.m_PropertyDesc.m_sName    = pin.m_sName;
        pin.m_PropertyDesc.m_Category = xiiPropertyCategory::Member;
        pin.m_PropertyDesc.m_Flags.SetValue((xiiUInt16)xiiPropertyFlags::Phantom | (xiiUInt16)xiiPropertyFlags::StandardType);
        pin.m_PropertyDesc.m_sType = pin.m_pDataType->GetTypeName();

        const xiiVariant def = ExtractDefaultValue(pin.m_pDataType, pin.m_sDefaultValue);

        if (def.IsValid())
        {
          pin.m_PropertyDesc.m_Attributes.PushBack(XII_DEFAULT_NEW(xiiDefaultValueAttribute, def));
        }
      }

      pinArray.PushBack(pin);
    }
  }
}

void xiiVisualShaderTypeRegistry::ExtractNodeProperties(const xiiOpenDdlReaderElement* pNode, xiiVisualShaderNodeDescriptor& nd)
{
  for (const xiiOpenDdlReaderElement* pElement = pNode->GetFirstChild(); pElement != nullptr; pElement = pElement->GetSibling())
  {
    if (pElement->GetCustomType() == "Property")
    {
      xiiInt8 iValueGroup = -1;

      xiiReflectedPropertyDescriptor prop;
      prop.m_Category = xiiPropertyCategory::Member;
      prop.m_Flags.SetValue((xiiUInt16)xiiPropertyFlags::Phantom | (xiiUInt16)xiiPropertyFlags::StandardType);

      if (!pElement->HasName())
      {
        xiiLog::Error("Property doesn't have a name");
        continue;
      }

      prop.m_sName = pElement->GetName();

      const xiiOpenDdlReaderElement* pType = pElement->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Type");
      if (!pType)
      {
        xiiLog::Error("Property doesn't have a type");
        continue;
      }

      const xiiRTTI* pRtti = nullptr;

      {
        const xiiStringView& sType = pType->GetPrimitivesString()[0];

        if (sType == "color")
        {
          pRtti = xiiGetStaticRTTI<xiiColor>();

          // always expose the alpha channel for color properties
          xiiExposeColorAlphaAttribute* pAttr = xiiExposeColorAlphaAttribute::GetStaticRTTI()->GetAllocator()->Allocate<xiiExposeColorAlphaAttribute>();
          prop.m_Attributes.PushBack(pAttr);
        }
        else if (sType == "float4")
        {
          pRtti = xiiGetStaticRTTI<xiiVec4>();
        }
        else if (sType == "float3")
        {
          pRtti = xiiGetStaticRTTI<xiiVec3>();
        }
        else if (sType == "float2")
        {
          pRtti = xiiGetStaticRTTI<xiiVec2>();
        }
        else if (sType == "float")
        {
          pRtti = xiiGetStaticRTTI<float>();
        }
        else if (sType == "int")
        {
          pRtti = xiiGetStaticRTTI<int>();
        }
        else if (sType == "bool")
        {
          pRtti = xiiGetStaticRTTI<bool>();
        }
        else if (sType == "string")
        {
          pRtti = xiiGetStaticRTTI<xiiString>();
        }
        else if (sType == "identifier")
        {
          pRtti = xiiGetStaticRTTI<xiiString>();

          iValueGroup = 1; // currently no way to specify the group
        }
        else if (sType == "Texture2D")
        {
          pRtti = xiiGetStaticRTTI<xiiString>();

          // apparently the attributes are deallocated using the type allocator, so we must allocate them here through RTTI as well
          xiiAssetBrowserAttribute* pAttr = xiiAssetBrowserAttribute::GetStaticRTTI()->GetAllocator()->Allocate<xiiAssetBrowserAttribute>();
          pAttr->SetTypeFilter("CompatibleAsset_Texture_2D");
          prop.m_Attributes.PushBack(pAttr);
        }
        else
        {
          xiiLog::Error("Invalid property type '{0}'", sType);
          continue;
        }
      }

      prop.m_sType = pRtti->GetTypeName();

      const xiiOpenDdlReaderElement* pValue = pElement->FindChild("DefaultValue");
      if (pValue && pRtti != nullptr && pValue->HasPrimitives(xiiOpenDdlPrimitiveType::String))
      {
        xiiStringBuilder tmp = pValue->GetPrimitivesString()[0];
        const xiiVariant def = ExtractDefaultValue(pRtti, tmp);

        if (def.IsValid())
        {
          prop.m_Attributes.PushBack(XII_DEFAULT_NEW(xiiDefaultValueAttribute, def));
        }
      }

      nd.m_Properties.PushBack(prop);
      nd.m_UniquePropertyValueGroups.PushBack(iValueGroup);
    }
  }
}

void xiiVisualShaderTypeRegistry::ExtractNodeConfig(const xiiOpenDdlReaderElement* pNode, xiiVisualShaderNodeDescriptor& nd)
{
  xiiStringBuilder temp;

  const xiiOpenDdlReaderElement* pElement = pNode->GetFirstChild();

  while (pElement)
  {
    if (pElement->GetName() == "Color")
    {
      GetColorFromDdl(pElement, nd.m_Color);
    }
    else if (pElement->HasPrimitives(xiiOpenDdlPrimitiveType::String))
    {
      if (pElement->GetName() == "NodeType")
      {
        if (pElement->GetPrimitivesString()[0] == "Main")
          nd.m_NodeType = xiiVisualShaderNodeType::Main;
        else if (pElement->GetPrimitivesString()[0] == "Texture")
          nd.m_NodeType = xiiVisualShaderNodeType::Texture;
        else
          nd.m_NodeType = xiiVisualShaderNodeType::Generic;
      }
      else if (pElement->GetName() == "Category")
      {
        nd.m_sCategory.Assign(pElement->GetPrimitivesString()[0]);
      }
      else if (pElement->GetName() == "CheckPermutations")
      {
        temp = pElement->GetPrimitivesString()[0];
        temp.ReplaceAll(" ", "");
        temp.ReplaceAll("\r", "");
        temp.ReplaceAll("\t", "");
        temp.Trim("\n");
        nd.m_sCheckPermutations = temp;
      }
      else if (pElement->GetName() == "CodePermutations")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePermutations = temp;
      }
      else if (pElement->GetName() == "CodeRenderStates")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeRenderState = temp;
      }
      else if (pElement->GetName() == "CodeVertexShader")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeVertexShader = temp;
      }
      else if (pElement->GetName() == "CodeGeometryShader")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeGeometryShader = temp;
      }
      else if (pElement->GetName() == "CodeMaterialParams")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeMaterialParams = temp;
      }
      else if (pElement->GetName() == "CodeMaterialCB")
      {
        temp                       = pElement->GetPrimitivesString()[0];
        nd.m_sShaderCodeMaterialCB = temp;
      }
      else if (pElement->GetName() == "CodePixelDefines")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelDefines = temp;
      }
      else if (pElement->GetName() == "CodePixelIncludes")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelIncludes = temp;
      }
      else if (pElement->GetName() == "CodePixelSamplers")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelSamplers = temp;
      }
      else if (pElement->GetName() == "CodePixelConstants")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelConstants = temp;
      }
      else if (pElement->GetName() == "CodePixelBody")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelBody = temp;
      }
    }

    pElement = pElement->GetSibling();
  }
}

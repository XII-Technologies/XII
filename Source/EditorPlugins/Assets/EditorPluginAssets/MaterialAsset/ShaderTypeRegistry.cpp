/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/ShaderTypeRegistry.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderParser.h>

XII_IMPLEMENT_SINGLETON(xiiShaderTypeRegistry);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, ShaderTypeRegistry)

BEGIN_SUBSYSTEM_DEPENDENCIES
  "ReflectedTypeManager"
END_SUBSYSTEM_DEPENDENCIES

ON_CORESYSTEMS_STARTUP
{
  XII_DEFAULT_NEW(xiiShaderTypeRegistry);
}

ON_CORESYSTEMS_SHUTDOWN
{
  xiiShaderTypeRegistry* pDummy = xiiShaderTypeRegistry::GetSingleton();
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
  struct PermutationVarConfig
  {
    xiiVariant     m_DefaultValue;
    const xiiRTTI* m_pType;
  };

  static xiiHashTable<xiiString, PermutationVarConfig> s_PermutationVarConfigs;
  static xiiHashTable<xiiString, const xiiRTTI*>       s_EnumTypes;

  const xiiRTTI* GetPermutationType(const xiiGALShaderParser::ParameterDefinition& def)
  {
    XII_ASSERT_DEV(def.m_sType.IsEqual("Permutation"), "");

    PermutationVarConfig* pConfig = nullptr;
    if (s_PermutationVarConfigs.TryGetValue(def.m_sName, pConfig))
    {
      return pConfig->m_pType;
    }

    xiiStringBuilder sTemp;
    sTemp.SetFormat("Shaders/PermutationVariables/{0}.xiiPermVar", def.m_sName);

    xiiString sPath = sTemp;
    xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);

    xiiFileReader file;
    if (file.Open(sPath).Failed())
    {
      return nullptr;
    }

    sTemp.ReadAll(file);

    xiiVariant                         defaultValue;
    xiiGALShaderParser::EnumDefinition enumDefinition;

    xiiGALShaderParser::ParsePermutationVariableConfiguration(sTemp, defaultValue, enumDefinition);
    if (defaultValue.IsValid())
    {
      pConfig                 = &(s_PermutationVarConfigs[def.m_sName]);
      pConfig->m_DefaultValue = defaultValue;

      if (defaultValue.IsA<bool>())
      {
        pConfig->m_pType = xiiGetStaticRTTI<bool>();
      }
      else
      {
        xiiReflectedTypeDescriptor descEnum;
        descEnum.m_sTypeName       = def.m_sName;
        descEnum.m_sPluginName     = "ShaderTypes";
        descEnum.m_sParentTypeName = xiiGetStaticRTTI<xiiEnumBase>()->GetTypeName();
        descEnum.m_Flags           = xiiTypeFlags::IsEnum | xiiTypeFlags::Phantom;
        descEnum.m_uiTypeVersion   = 1;

        xiiArrayPtr<xiiPropertyAttribute* const> noAttributes;

        xiiStringBuilder sEnumName;
        sEnumName.SetFormat("{0}::Default", def.m_sName);

        descEnum.m_Properties.PushBack(xiiReflectedPropertyDescriptor(sEnumName, defaultValue.Get<xiiUInt32>(), noAttributes));

        for (const auto& ev : enumDefinition.m_Values)
        {
          xiiStringBuilder sEnumName;
          sEnumName.SetFormat("{0}::{1}", def.m_sName, ev.m_sValueName);

          descEnum.m_Properties.PushBack(xiiReflectedPropertyDescriptor(sEnumName, ev.m_iValueValue, noAttributes));
        }

        pConfig->m_pType = xiiPhantomRttiManager::RegisterType(descEnum);
      }

      return pConfig->m_pType;
    }

    return nullptr;
  }

  const xiiRTTI* GetEnumType(const xiiGALShaderParser::EnumDefinition& def)
  {
    const xiiRTTI* pType = nullptr;
    if (s_EnumTypes.TryGetValue(def.m_sName, pType))
    {
      return pType;
    }

    xiiReflectedTypeDescriptor descEnum;
    descEnum.m_sTypeName       = def.m_sName;
    descEnum.m_sPluginName     = "ShaderTypes";
    descEnum.m_sParentTypeName = xiiGetStaticRTTI<xiiEnumBase>()->GetTypeName();
    descEnum.m_Flags           = xiiTypeFlags::IsEnum | xiiTypeFlags::Phantom;
    descEnum.m_uiTypeVersion   = 1;

    xiiArrayPtr<xiiPropertyAttribute* const> noAttributes;

    xiiStringBuilder sEnumName;
    sEnumName.SetFormat("{0}::Default", def.m_sName);

    descEnum.m_Properties.PushBack(xiiReflectedPropertyDescriptor(sEnumName, def.m_uiDefaultValue, noAttributes));

    for (const auto& ev : def.m_Values)
    {
      xiiStringBuilder sEnumName;
      sEnumName.SetFormat("{0}::{1}", def.m_sName, ev.m_sValueName);

      descEnum.m_Properties.PushBack(xiiReflectedPropertyDescriptor(sEnumName, ev.m_iValueValue, noAttributes));
    }

    pType = xiiPhantomRttiManager::RegisterType(descEnum);

    s_EnumTypes.Insert(def.m_sName, pType);

    return pType;
  }

  const xiiRTTI* GetType(const xiiGALShaderParser::ParameterDefinition& def)
  {
    if (def.m_pType != nullptr)
    {
      return def.m_pType;
    }

    if (def.m_sType.IsEqual("Permutation"))
    {
      return GetPermutationType(def);
    }

    const xiiRTTI* pType = nullptr;
    s_EnumTypes.TryGetValue(def.m_sType, pType);

    return pType;
  }

  void AddAttributes(xiiGALShaderParser::ParameterDefinition& ref_def, const xiiRTTI* pType, xiiHybridArray<const xiiPropertyAttribute*, 2>& ref_attributes)
  {
    if (ref_def.m_sType.StartsWith_NoCase("texture"))
    {
      if (ref_def.m_sType.IsEqual("Texture2D"))
      {
        ref_attributes.PushBack(XII_DEFAULT_NEW(xiiCategoryAttribute, "Texture 2D"));
        ref_attributes.PushBack(XII_DEFAULT_NEW(xiiAssetBrowserAttribute, "CompatibleAsset_Texture_2D"));
      }
      else if (ref_def.m_sType.IsEqual("Texture3D"))
      {
        ref_attributes.PushBack(XII_DEFAULT_NEW(xiiCategoryAttribute, "Texture 3D"));
        ref_attributes.PushBack(XII_DEFAULT_NEW(xiiAssetBrowserAttribute, "CompatibleAsset_Texture_3D"));
      }
      else if (ref_def.m_sType.IsEqual("TextureCube"))
      {
        ref_attributes.PushBack(XII_DEFAULT_NEW(xiiCategoryAttribute, "Texture Cube"));
        ref_attributes.PushBack(XII_DEFAULT_NEW(xiiAssetBrowserAttribute, "CompatibleAsset_Texture_Cube"));
      }
    }
    else if (ref_def.m_sType.StartsWith_NoCase("permutation"))
    {
      ref_attributes.PushBack(XII_DEFAULT_NEW(xiiCategoryAttribute, "Permutation"));
    }
    else
    {
      ref_attributes.PushBack(XII_DEFAULT_NEW(xiiCategoryAttribute, "Constant"));
    }

    for (auto& attributeDef : ref_def.m_Attributes)
    {
      if (attributeDef.m_sType.IsEqual("Default") && attributeDef.m_Values.GetCount() >= 1)
      {
        if (pType == xiiGetStaticRTTI<xiiColor>())
        {
          // always expose the alpha channel for color properties
          ref_attributes.PushBack(XII_DEFAULT_NEW(xiiExposeColorAlphaAttribute));

          // patch default type, VSE writes float4 instead of color
          if (attributeDef.m_Values[0].GetType() == xiiVariantType::Vector4)
          {
            xiiVec4 v                = attributeDef.m_Values[0].Get<xiiVec4>();
            attributeDef.m_Values[0] = xiiColor(v.x, v.y, v.z, v.w);
          }
        }

        ref_attributes.PushBack(XII_DEFAULT_NEW(xiiDefaultValueAttribute, attributeDef.m_Values[0]));
      }
      else if (attributeDef.m_sType.IsEqual("Clamp") && attributeDef.m_Values.GetCount() >= 2)
      {
        ref_attributes.PushBack(XII_DEFAULT_NEW(xiiClampValueAttribute, attributeDef.m_Values[0], attributeDef.m_Values[1]));
      }
      else if (attributeDef.m_sType.IsEqual("Group"))
      {
        if (attributeDef.m_Values.GetCount() >= 1 && attributeDef.m_Values[0].CanConvertTo<xiiString>())
        {
          ref_attributes.PushBack(XII_DEFAULT_NEW(xiiGroupAttribute, attributeDef.m_Values[0].ConvertTo<xiiString>()));
        }
        else
        {
          ref_attributes.PushBack(XII_DEFAULT_NEW(xiiGroupAttribute));
        }
      }
    }
  }
} // namespace

xiiShaderTypeRegistry::xiiShaderTypeRegistry() :
  m_SingletonRegistrar(this)
{
  xiiShaderTypeRegistry::GetSingleton();
  xiiReflectedTypeDescriptor desc;
  desc.m_sTypeName       = "xiiShaderTypeBase";
  desc.m_sPluginName     = "ShaderTypes";
  desc.m_sParentTypeName = xiiGetStaticRTTI<xiiReflectedClass>()->GetTypeName();
  desc.m_Flags           = xiiTypeFlags::Phantom | xiiTypeFlags::Abstract | xiiTypeFlags::Class;
  desc.m_uiTypeVersion   = 2;

  m_pBaseType = xiiPhantomRttiManager::RegisterType(desc);

  xiiPhantomRttiManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
}


xiiShaderTypeRegistry::~xiiShaderTypeRegistry()
{
  xiiPhantomRttiManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
}

const xiiRTTI* xiiShaderTypeRegistry::GetShaderType(xiiStringView sShaderPath0)
{
  if (sShaderPath0.IsEmpty())
    return nullptr;

  xiiStringBuilder sShaderPath = sShaderPath0;
  sShaderPath.MakeCleanPath();

  if (sShaderPath.IsAbsolutePath())
  {
    if (!xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sShaderPath))
    {
      xiiLog::Error("Could not make shader path '{0}' relative!", sShaderPath);
    }
  }

  auto it = m_ShaderTypes.Find(sShaderPath);
  if (it.IsValid())
  {
    xiiFileStats Stats;
    if (xiiOSFile::GetFileStats(it.Value().m_sAbsShaderPath, Stats).Succeeded() &&
        !Stats.m_LastModificationTime.Compare(it.Value().m_fileModifiedTime, xiiTimestamp::CompareMode::FileTimeEqual))
    {
      UpdateShaderType(it.Value());
    }
  }
  else
  {
    xiiStringBuilder sAbsPath = sShaderPath0;
    {
      if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
      {
        xiiLog::Warning("Can't make path absolute: '{0}'", sShaderPath0);
        return nullptr;
      }
      sAbsPath.MakeCleanPath();
    }

    it                          = m_ShaderTypes.Insert(sShaderPath, ShaderData());
    it.Value().m_sShaderPath    = sShaderPath;
    it.Value().m_sAbsShaderPath = sAbsPath;
    UpdateShaderType(it.Value());
  }

  return it.Value().m_pType;
}

void xiiShaderTypeRegistry::UpdateShaderType(ShaderData& data)
{
  XII_LOG_BLOCK("Updating Shader Parameters", data.m_sShaderPath.GetView());

  xiiHybridArray<xiiGALShaderParser::ParameterDefinition, 16> parameters;
  xiiHybridArray<xiiGALShaderParser::EnumDefinition, 4>       enumDefinitions;

  {
    xiiFileStats Stats;
    bool         bStat = xiiOSFile::GetFileStats(data.m_sAbsShaderPath, Stats).Succeeded();

    xiiFileReader file;
    if (!bStat || file.Open(data.m_sAbsShaderPath).Failed())
    {
      xiiLog::Error("Can't update shader '{0}' type information, the file can't be opened.", data.m_sShaderPath);
      return;
    }

    xiiGALShaderParser::ParseMaterialParameterSection(file, parameters, enumDefinitions);
    data.m_fileModifiedTime = Stats.m_LastModificationTime;
  }

  xiiReflectedTypeDescriptor desc;
  desc.m_sTypeName       = data.m_sShaderPath;
  desc.m_sPluginName     = "ShaderTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags           = xiiTypeFlags::Phantom | xiiTypeFlags::Class;
  desc.m_uiTypeVersion   = 2;

  for (auto& enumDef : enumDefinitions)
  {
    GetEnumType(enumDef);
  }

  for (auto& parameter : parameters)
  {
    const xiiRTTI* pType = GetType(parameter);
    if (pType == nullptr)
    {
      continue;
    }

    xiiBitflags<xiiPropertyFlags> flags = xiiPropertyFlags::Phantom;
    if (pType->IsDerivedFrom<xiiEnumBase>())
      flags |= xiiPropertyFlags::IsEnum;
    if (pType->IsDerivedFrom<xiiBitflagsBase>())
      flags |= xiiPropertyFlags::Bitflags;
    if (xiiReflectionUtils::IsBasicType(pType))
      flags |= xiiPropertyFlags::StandardType;

    xiiReflectedPropertyDescriptor propDesc(xiiPropertyCategory::Member, parameter.m_sName, pType->GetTypeName(), flags);

    AddAttributes(parameter, pType, propDesc.m_Attributes);

    desc.m_Properties.PushBack(propDesc);
  }

  // Register and return the phantom type. If the type already exists this will update the type
  // and patch any existing instances of it so they should show up in the prop grid right away.
  xiiPhantomRttiManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
  {
    // We do not want to listen to type changes that we triggered ourselves.
    data.m_pType = xiiPhantomRttiManager::RegisterType(desc);
  }
  xiiPhantomRttiManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiShaderTypeRegistry::PhantomTypeRegistryEventHandler, this));
}

void xiiShaderTypeRegistry::PhantomTypeRegistryEventHandler(const xiiPhantomRttiManagerEvent& e)
{
  if (e.m_Type == xiiPhantomRttiManagerEvent::Type::TypeAdded)
  {
    if (e.m_pChangedType->GetParentType() == m_pBaseType)
    {
      GetShaderType(e.m_pChangedType->GetTypeName());
    }
  }
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

/// Changes the base class of all shader types to xiiShaderTypeBase (version 1) and
/// sets their own version to 2.
class xiiShaderTypePatch_1_2 : public xiiGraphPatch
{
public:
  xiiShaderTypePatch_1_2() :
    xiiGraphPatch(nullptr, 2, xiiGraphPatch::PatchType::GraphPatch)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode*) const override
  {
    xiiString sDescTypeName = xiiGetStaticRTTI<xiiReflectedTypeDescriptor>()->GetTypeName();

    auto& nodes             = pGraph->GetAllNodes();
    bool  bNeedAddBaseClass = false;
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      xiiAbstractObjectNode* pNode = it.Value();
      if (pNode->GetType() == sDescTypeName)
      {
        auto* pTypeProperty = pNode->FindProperty("TypeName");
        if (xiiStringUtils::EndsWith(pTypeProperty->m_Value.Get<xiiString>(), ".xiiShader"))
        {
          auto* pTypeVersionProperty = pNode->FindProperty("TypeVersion");
          auto* pParentTypeProperty  = pNode->FindProperty("ParentTypeName");
          if (pTypeVersionProperty->m_Value == 1)
          {
            pParentTypeProperty->m_Value  = "xiiShaderTypeBase";
            pTypeVersionProperty->m_Value = (xiiUInt32)2;
            bNeedAddBaseClass             = true;
          }
        }
      }
    }

    if (bNeedAddBaseClass)
    {
      xiiRttiConverterContext context;
      xiiRttiConverterWriter  rttiConverter(pGraph, &context, true, true);

      xiiReflectedTypeDescriptor desc;
      desc.m_sTypeName       = "xiiShaderTypeBase";
      desc.m_sPluginName     = "ShaderTypes";
      desc.m_sParentTypeName = xiiGetStaticRTTI<xiiReflectedClass>()->GetTypeName();
      desc.m_Flags           = xiiTypeFlags::Phantom | xiiTypeFlags::Abstract | xiiTypeFlags::Class;
      desc.m_uiTypeVersion   = 1;

      context.RegisterObject(xiiUuid::MakeStableUuidFromString(desc.m_sTypeName.GetView()), xiiGetStaticRTTI<xiiReflectedTypeDescriptor>(), &desc);
      rttiConverter.AddObjectToGraph(xiiGetStaticRTTI<xiiReflectedTypeDescriptor>(), &desc);
    }
  }
};

xiiShaderTypePatch_1_2 g_xiiShaderTypePatch_1_2;

// TODO: Increase xiiShaderTypeBase version to 2 and implement enum renames, see xiiReflectedPropertyDescriptorPatch_1_2
class xiiShaderBaseTypePatch_1_2 : public xiiGraphPatch
{
public:
  xiiShaderBaseTypePatch_1_2() :
    xiiGraphPatch("xiiShaderTypeBase", 2)
  {
  }

  static void FixEnumString(xiiStringBuilder& ref_sValue, const char* szName)
  {
    if (ref_sValue.StartsWith(szName))
      ref_sValue.Shrink(xiiStringUtils::GetCharacterCount(szName), 0);

    if (ref_sValue.StartsWith("::"))
      ref_sValue.Shrink(2, 0);

    if (ref_sValue.StartsWith(szName))
      ref_sValue.Shrink(xiiStringUtils::GetCharacterCount(szName), 0);

    if (ref_sValue.StartsWith("_"))
      ref_sValue.Shrink(1, 0);

    ref_sValue.PrependFormat("{0}::{0}_", szName);
  }

  void FixEnum(xiiAbstractObjectNode* pNode, const char* szEnum) const
  {
    if (xiiAbstractObjectNode::Property* pProp = pNode->FindProperty(szEnum))
    {
      xiiStringBuilder sValue = pProp->m_Value.Get<xiiString>();
      FixEnumString(sValue, szEnum);
      pProp->m_Value = sValue.GetData();
    }
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    FixEnum(pNode, "SHADING_MODE");
    FixEnum(pNode, "BLEND_MODE");
    FixEnum(pNode, "RENDER_PASS");
  }
};

xiiShaderBaseTypePatch_1_2 g_xiiShaderBaseTypePatch_1_2;

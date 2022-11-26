#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/ExposedParametersTypeRegistry.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

XII_IMPLEMENT_SINGLETON(xiiExposedParametersTypeRegistry);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, ExposedParametersTypeRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager", "AssetCurator"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiExposedParametersTypeRegistry);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiExposedParametersTypeRegistry* pDummy = xiiExposedParametersTypeRegistry::GetSingleton();
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

xiiExposedParametersTypeRegistry::xiiExposedParametersTypeRegistry() :
  m_SingletonRegistrar(this)
{
  xiiReflectedTypeDescriptor desc;
  desc.m_sTypeName       = "xiiExposedParametersTypeBase";
  desc.m_sPluginName     = "ExposedParametersTypes";
  desc.m_sParentTypeName = xiiGetStaticRTTI<xiiReflectedClass>()->GetTypeName();
  desc.m_Flags           = xiiTypeFlags::Phantom | xiiTypeFlags::Abstract | xiiTypeFlags::Class;
  desc.m_uiTypeVersion   = 0;

  m_pBaseType = xiiPhantomRttiManager::RegisterType(desc);

  xiiAssetCurator::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiExposedParametersTypeRegistry::AssetCuratorEventHandler, this));
  xiiPhantomRttiManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiExposedParametersTypeRegistry::PhantomTypeRegistryEventHandler, this));
}


xiiExposedParametersTypeRegistry::~xiiExposedParametersTypeRegistry()
{
  xiiAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiExposedParametersTypeRegistry::AssetCuratorEventHandler, this));
  xiiPhantomRttiManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiExposedParametersTypeRegistry::PhantomTypeRegistryEventHandler, this));
}

const xiiRTTI* xiiExposedParametersTypeRegistry::GetExposedParametersType(const char* szResource)
{
  if (xiiStringUtils::IsNullOrEmpty(szResource))
    return nullptr;

  const auto asset = xiiAssetCurator::GetSingleton()->FindSubAsset(szResource);
  if (!asset)
    return nullptr;

  auto params = asset->m_pAssetInfo->m_Info->GetMetaInfo<xiiExposedParameters>();
  if (!params)
    return nullptr;

  auto it = m_ShaderTypes.Find(asset->m_Data.m_Guid);
  if (it.IsValid())
  {
    if (!it.Value().m_bUpToDate)
    {
      UpdateExposedParametersType(it.Value(), *params);
    }
  }
  else
  {
    it                        = m_ShaderTypes.Insert(asset->m_Data.m_Guid, ParamData());
    it.Value().m_SubAssetGuid = asset->m_Data.m_Guid;
    UpdateExposedParametersType(it.Value(), *params);
  }

  return it.Value().m_pType;
}

void xiiExposedParametersTypeRegistry::UpdateExposedParametersType(ParamData& data, const xiiExposedParameters& params)
{
  xiiStringBuilder name;
  name.Format("xiiExposedParameters_{0}", data.m_SubAssetGuid);
  XII_LOG_BLOCK("Updating Type", name.GetData());
  xiiReflectedTypeDescriptor desc;
  desc.m_sTypeName       = name;
  desc.m_sPluginName     = "ExposedParametersTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags           = xiiTypeFlags::Phantom | xiiTypeFlags::Class;
  desc.m_uiTypeVersion   = 2;

  for (const auto* parameter : params.m_Parameters)
  {
    const xiiRTTI* pType = xiiReflectionUtils::GetTypeFromVariant(parameter->m_DefaultValue);
    if (!parameter->m_sType.IsEmpty())
    {
      if (const xiiRTTI* pType2 = xiiRTTI::FindTypeByName(parameter->m_sType))
        pType = pType2;
    }
    if (pType == nullptr)
      continue;

    xiiBitflags<xiiPropertyFlags> flags = xiiPropertyFlags::Phantom;
    if (pType->IsDerivedFrom<xiiEnumBase>())
      flags |= xiiPropertyFlags::IsEnum;
    if (pType->IsDerivedFrom<xiiBitflagsBase>())
      flags |= xiiPropertyFlags::Bitflags;
    if (xiiReflectionUtils::IsBasicType(pType))
      flags |= xiiPropertyFlags::StandardType;
    else
      flags |= xiiPropertyFlags::Class;

    xiiReflectedPropertyDescriptor propDesc(xiiPropertyCategory::Member, parameter->m_sName, pType->GetTypeName(), flags);
    for (auto attrib : parameter->m_Attributes)
    {
      propDesc.m_Attributes.PushBack(xiiReflectionSerializer::Clone(attrib));
    }
    desc.m_Properties.PushBack(propDesc);
  }

  // Register and return the phantom type. If the type already exists this will update the type
  // and patch any existing instances of it so they should show up in the prop grid right away.
  {
    // This fkt is called by the property grid, but calling RegisterType will update the property grid
    // and we will recurse into this. So we listen for the xiiPhantomRttiManager events to fill out
    // the data.m_pType in it to make sure recursion into GetExposedParametersType does not return a nullptr.
    m_pAboutToBeRegistered = &data;
    data.m_bUpToDate       = true;
    data.m_pType           = xiiPhantomRttiManager::RegisterType(desc);
    m_pAboutToBeRegistered = nullptr;
  }
}

void xiiExposedParametersTypeRegistry::AssetCuratorEventHandler(const xiiAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case xiiAssetCuratorEvent::Type::AssetRemoved:
    {
      // Ignore for now, doesn't hurt. Removing types is more hassle than it is worth.
      if (auto* data = m_ShaderTypes.GetValue(e.m_AssetGuid))
      {
        data->m_bUpToDate = false;
      }
    }
    break;
    case xiiAssetCuratorEvent::Type::AssetListReset:
    {
      for (auto it = m_ShaderTypes.GetIterator(); it.IsValid(); ++it)
      {
        it.Value().m_bUpToDate = false;
      }
    }
    break;
    case xiiAssetCuratorEvent::Type::AssetUpdated:
    {
      if (auto* data = m_ShaderTypes.GetValue(e.m_AssetGuid))
      {
        data->m_bUpToDate = false;
        if (auto params = e.m_pInfo->m_pAssetInfo->m_Info->GetMetaInfo<xiiExposedParameters>())
          UpdateExposedParametersType(*data, *params);
      }
    }
    break;
    default:
      break;
  }
}

void xiiExposedParametersTypeRegistry::PhantomTypeRegistryEventHandler(const xiiPhantomRttiManagerEvent& e)
{
  if (e.m_Type == xiiPhantomRttiManagerEvent::Type::TypeAdded || e.m_Type == xiiPhantomRttiManagerEvent::Type::TypeChanged)
  {
    if (e.m_pChangedType->GetParentType() == m_pBaseType && m_pAboutToBeRegistered)
    {
      // We listen for the xiiPhantomRttiManager events to fill out the m_pType pointer. This is needed as otherwise
      // Recursion into GetExposedParametersType would return a nullptr.
      m_pAboutToBeRegistered->m_pType = e.m_pChangedType;
    }
  }
}

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/RttiConverter.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTypeVersionInfo, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTypeVersionInfo>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("TypeName", GetTypeName, SetTypeName),
    XII_ACCESSOR_PROPERTY("ParentTypeName", GetParentTypeName, SetParentTypeName),
    XII_MEMBER_PROPERTY("TypeVersion", m_uiTypeVersion),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiStringView xiiTypeVersionInfo::GetTypeName() const
{
  return m_sTypeName.GetView();
}

void xiiTypeVersionInfo::SetTypeName(xiiStringView sName)
{
  m_sTypeName.Assign(sName);
}

xiiStringView xiiTypeVersionInfo::GetParentTypeName() const
{
  return m_sParentTypeName.GetData();
}

void xiiTypeVersionInfo::SetParentTypeName(xiiStringView sName)
{
  m_sParentTypeName.Assign(sName);
}

void xiiGraphPatchContext::PatchBaseClass(xiiStringView sType, xiiUInt32 uiTypeVersion, bool bForcePatch)
{
  xiiHashedString sTypeHash;
  sTypeHash.Assign(sType);
  for (xiiUInt32 uiBaseClassIndex = m_uiBaseClassIndex; uiBaseClassIndex < m_BaseClasses.GetCount(); ++uiBaseClassIndex)
  {
    if (m_BaseClasses[uiBaseClassIndex].m_sType == sTypeHash)
    {
      Patch(uiBaseClassIndex, uiTypeVersion, bForcePatch);
      return;
    }
  }
  XII_REPORT_FAILURE("Base class of name '{0}' not found in parent types of '{1}'", sTypeHash.GetView(), m_pNode->GetType());
}

void xiiGraphPatchContext::RenameClass(xiiStringView sTypeName)
{
  m_pNode->SetType(m_pGraph->RegisterString(sTypeName));
  m_BaseClasses[m_uiBaseClassIndex].m_sType.Assign(sTypeName);
}

void xiiGraphPatchContext::RenameClass(xiiStringView sTypeName, xiiUInt32 uiVersion)
{
  m_pNode->SetType(m_pGraph->RegisterString(sTypeName));
  m_BaseClasses[m_uiBaseClassIndex].m_sType.Assign(sTypeName);
  // After a Patch is applied, the version is always increased. So if we want to change the version we need to reduce it by one so that in the next patch loop the requested version is not skipped.
  XII_ASSERT_DEV(uiVersion > 0, "Cannot change the version of a class to 0, target version must be at least 1.");
  m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion = uiVersion - 1;
}

void xiiGraphPatchContext::ChangeBaseClass(xiiArrayPtr<xiiVersionKey> baseClasses)
{
  m_BaseClasses.SetCount(m_uiBaseClassIndex + 1 + baseClasses.GetCount());
  for (xiiUInt32 i = 0; i < baseClasses.GetCount(); i++)
  {
    m_BaseClasses[m_uiBaseClassIndex + 1 + i] = baseClasses[i];
  }
}

//////////////////////////////////////////////////////////////////////////

xiiGraphPatchContext::xiiGraphPatchContext(xiiGraphVersioning* pParent, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectGraph* pTypesGraph)
{
  XII_PROFILE_SCOPE("xiiGraphPatchContext");

  m_pParent = pParent;
  m_pGraph  = pGraph;

  if (pTypesGraph)
  {
    xiiRttiConverterContext context;
    xiiRttiConverterReader  rttiConverter(pTypesGraph, &context);
    xiiString               sDescTypeName = "xiiReflectedTypeDescriptor";
    auto&                   nodes         = pTypesGraph->GetAllNodes();

    m_TypeToInfo.Reserve(nodes.GetCount());

    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->GetType() == sDescTypeName)
      {
        xiiTypeVersionInfo info;
        rttiConverter.ApplyPropertiesToObject(it.Value(), xiiGetStaticRTTI<xiiTypeVersionInfo>(), &info);
        m_TypeToInfo.Insert(info.m_sTypeName, info);
      }
    }
  }
}

void xiiGraphPatchContext::Patch(xiiAbstractObjectNode* pNode)
{
  m_pNode = pNode;
  // Build version hierarchy.
  m_BaseClasses.Clear();

  xiiVersionKey key;
  key.m_sType.Assign(m_pNode->GetType());
  key.m_uiTypeVersion = m_pNode->GetTypeVersion();

  m_BaseClasses.PushBack(key);
  UpdateBaseClasses();

  // Patch
  for (m_uiBaseClassIndex = 0; m_uiBaseClassIndex < m_BaseClasses.GetCount(); ++m_uiBaseClassIndex)
  {
    const xiiUInt32 uiMaxVersion = m_pParent->GetMaxPatchVersion(m_BaseClasses[m_uiBaseClassIndex].m_sType);
    Patch(m_uiBaseClassIndex, uiMaxVersion, false);
  }
  m_pNode->SetTypeVersion(m_BaseClasses[0].m_uiTypeVersion);
}

void xiiGraphPatchContext::Patch(xiiUInt32 uiBaseClassIndex, xiiUInt32 uiTypeVersion, bool bForcePatch)
{
  if (bForcePatch)
  {
    m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion = xiiMath::Min(m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion, uiTypeVersion - 1);
  }
  while (m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion < uiTypeVersion)
  {
    // Don't move this out of the loop, needed to support renaming a class which will change the key.
    xiiVersionKey key = m_BaseClasses[uiBaseClassIndex];
    key.m_uiTypeVersion += 1;

    const xiiGraphPatch* pPatch = nullptr;
    if (m_pParent->m_NodePatches.TryGetValue(key, pPatch))
    {
      pPatch->Patch(*this, m_pGraph, m_pNode);
      uiTypeVersion = m_pParent->GetMaxPatchVersion(m_BaseClasses[m_uiBaseClassIndex].m_sType);
    }
    // Don't use a ref to the key as the array might get resized during patching.
    // Patch function can change the type and version so we need to read m_uiTypeVersion again instead of just writing key.m_uiTypeVersion;
    m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion++;
  }
}

void xiiGraphPatchContext::UpdateBaseClasses()
{
  for (;;)
  {
    xiiHashedString sParentType;
    if (xiiTypeVersionInfo* pInfo = m_TypeToInfo.GetValue(m_BaseClasses.PeekBack().m_sType))
    {
      m_BaseClasses.PeekBack().m_uiTypeVersion = pInfo->m_uiTypeVersion;
      sParentType                              = pInfo->m_sParentTypeName;
    }
    else if (const xiiRTTI* pType = xiiRTTI::FindTypeByName(m_BaseClasses.PeekBack().m_sType.GetData()))
    {
      m_BaseClasses.PeekBack().m_uiTypeVersion = pType->GetTypeVersion();
      if (pType->GetParentType())
      {
        sParentType.Assign(pType->GetParentType()->GetTypeName());
      }
      else
        sParentType = xiiHashedString();
    }
    else
    {
      xiiLog::Error("Can't patch base class, parent type of '{0}' unknown.", m_BaseClasses.PeekBack().m_sType.GetData());
      break;
    }

    if (sParentType.IsEmpty())
      break;

    xiiVersionKey key;
    key.m_sType         = std::move(sParentType);
    key.m_uiTypeVersion = 0;
    m_BaseClasses.PushBack(key);
  }
}

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_SINGLETON(xiiGraphVersioning);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, GraphVersioning)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Reflection"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiGraphVersioning);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGraphVersioning* pDummy = xiiGraphVersioning::GetSingleton();
    XII_DEFAULT_DELETE(pDummy);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiGraphVersioning::xiiGraphVersioning() :
  m_SingletonRegistrar(this)
{
  xiiPlugin::Events().AddEventHandler(xiiMakeDelegate(&xiiGraphVersioning::PluginEventHandler, this));

  UpdatePatches();
}

xiiGraphVersioning::~xiiGraphVersioning()
{
  xiiPlugin::Events().RemoveEventHandler(xiiMakeDelegate(&xiiGraphVersioning::PluginEventHandler, this));
}

void xiiGraphVersioning::PatchGraph(xiiAbstractObjectGraph* pGraph, xiiAbstractObjectGraph* pTypesGraph)
{
  XII_PROFILE_SCOPE("PatchGraph");

  xiiGraphPatchContext context(this, pGraph, pTypesGraph);
  for (const xiiGraphPatch* pPatch : m_GraphPatches)
  {
    pPatch->Patch(context, pGraph, nullptr);
  }

  auto& nodes = pGraph->GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    xiiAbstractObjectNode* pNode = it.Value();
    context.Patch(pNode);
  }
}

void xiiGraphVersioning::PluginEventHandler(const xiiPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case xiiPluginEvent::AfterLoadingBeforeInit:
    case xiiPluginEvent::AfterUnloading:
      UpdatePatches();
      break;
    default:
      break;
  }
}

void xiiGraphVersioning::UpdatePatches()
{
  m_GraphPatches.Clear();
  m_NodePatches.Clear();
  m_MaxPatchVersion.Clear();

  xiiVersionKey  key;
  xiiGraphPatch* pInstance = xiiGraphPatch::GetFirstInstance();

  while (pInstance)
  {
    switch (pInstance->GetPatchType())
    {
      case xiiGraphPatch::PatchType::NodePatch:
      {
        key.m_sType.Assign(pInstance->GetType());
        key.m_uiTypeVersion = pInstance->GetTypeVersion();
        m_NodePatches.Insert(key, pInstance);

        if (xiiUInt32* pMax = m_MaxPatchVersion.GetValue(key.m_sType))
        {
          *pMax = xiiMath::Max(*pMax, key.m_uiTypeVersion);
        }
        else
        {
          m_MaxPatchVersion[key.m_sType] = key.m_uiTypeVersion;
        }
      }
      break;
      case xiiGraphPatch::PatchType::GraphPatch:
      {
        m_GraphPatches.PushBack(pInstance);
      }
      break;
    }
    pInstance = pInstance->GetNextInstance();
  }

  m_GraphPatches.Sort([](const xiiGraphPatch* a, const xiiGraphPatch* b) -> bool { return a->GetTypeVersion() < b->GetTypeVersion(); });
}

xiiUInt32 xiiGraphVersioning::GetMaxPatchVersion(const xiiHashedString& sType) const
{
  if (const xiiUInt32* pMax = m_MaxPatchVersion.GetValue(sType))
  {
    return *pMax;
  }
  return 0;
}

XII_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_GraphVersioning);

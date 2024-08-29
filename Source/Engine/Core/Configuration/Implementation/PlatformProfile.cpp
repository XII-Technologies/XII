#include <Core/CorePCH.h>

#include <Core/Configuration/PlatformProfile.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>

#include <Core/ResourceManager/ResourceManager.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProfileConfigData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

xiiProfileConfigData::xiiProfileConfigData()  = default;
xiiProfileConfigData::~xiiProfileConfigData() = default;

void xiiProfileConfigData::SaveRuntimeData(xiiChunkStreamWriter& ref_stream) const
{
  XII_IGNORE_UNUSED(ref_stream);
}

void xiiProfileConfigData::LoadRuntimeData(xiiChunkStreamReader& ref_stream)
{
  XII_IGNORE_UNUSED(ref_stream);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPlatformProfile, 1, xiiRTTIDefaultAllocator<xiiPlatformProfile>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("TargetPlatform", m_sTargetPlatform)->AddAttributes(new xiiDynamicStringEnumAttribute("TargetPlatformNames"), new xiiDefaultValueAttribute("Windows")),
    XII_ARRAY_MEMBER_PROPERTY("Configs", m_Configs)->AddFlags(xiiPropertyFlags::PointerOwner)->AddAttributes(new xiiContainerAttribute(false, false, false)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiPlatformProfile::xiiPlatformProfile() = default;

xiiPlatformProfile::~xiiPlatformProfile()
{
  Clear();
}

void xiiPlatformProfile::Clear()
{
  for (auto pType : m_Configs)
  {
    pType->GetDynamicRTTI()->GetAllocator()->Deallocate(pType);
  }

  m_Configs.Clear();
}

void xiiPlatformProfile::AddMissingConfigs()
{
  xiiRTTI::ForEachDerivedType<xiiProfileConfigData>(
    [this](const xiiRTTI* pRtti) {
      // find all types derived from xiiProfileConfigData
      bool bHasTypeAlready = false;

      // check whether we already have an instance of this type
      for (auto pType : m_Configs)
      {
        if (pType && pType->GetDynamicRTTI() == pRtti)
        {
          bHasTypeAlready = true;
          break;
        }
      }

      if (!bHasTypeAlready)
      {
        // if not, allocate one
        xiiProfileConfigData* pObject = pRtti->GetAllocator()->Allocate<xiiProfileConfigData>();
        XII_ASSERT_DEV(pObject != nullptr, "Invalid profile config");
        xiiReflectionUtils::SetAllMemberPropertiesToDefault(pRtti, pObject);

        m_Configs.PushBack(pObject);
      }
    },
    xiiRTTI::ForEachOptions::ExcludeNonAllocatable);

  // in case unknown configs were loaded from disk, remove them
  m_Configs.RemoveAndSwap(nullptr);

  // sort all configs alphabetically
  m_Configs.Sort([](const xiiProfileConfigData* lhs, const xiiProfileConfigData* rhs) -> bool { return lhs->GetDynamicRTTI()->GetTypeName().Compare(rhs->GetDynamicRTTI()->GetTypeName()) < 0; });
}

const xiiProfileConfigData* xiiPlatformProfile::GetTypeConfig(const xiiRTTI* pRtti) const
{
  for (const auto* pConfig : m_Configs)
  {
    if (pConfig->GetDynamicRTTI() == pRtti)
      return pConfig;
  }

  return nullptr;
}

xiiProfileConfigData* xiiPlatformProfile::GetTypeConfig(const xiiRTTI* pRtti)
{
  // reuse the const-version
  return const_cast<xiiProfileConfigData*>(((const xiiPlatformProfile*)this)->GetTypeConfig(pRtti));
}

xiiResult xiiPlatformProfile::SaveForRuntime(xiiStringView sFile) const
{
  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(sFile));

  xiiChunkStreamWriter chunk(file);

  chunk.BeginStream(1);

  for (auto* pConfig : m_Configs)
  {
    pConfig->SaveRuntimeData(chunk);
  }

  chunk.EndStream();

  return XII_SUCCESS;
}

xiiResult xiiPlatformProfile::LoadForRuntime(xiiStringView sFile)
{
  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(sFile));

  xiiChunkStreamReader chunk(file);

  chunk.BeginStream();

  while (chunk.GetCurrentChunk().m_bValid)
  {
    for (auto* pConfig : m_Configs)
    {
      pConfig->LoadRuntimeData(chunk);
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  ++m_uiLastModificationCounter;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Core, Core_Configuration_Implementation_PlatformProfile);

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFrameworkTest/AssetTestDocument.h>

xiiAtomicInteger32 ResaveTestManager::s_NonActiveOutputChecks;

bool ResaveTestManager::IsOutputUpToDate(xiiStringView path, xiiStringView tag, xiiUInt64 hash, const xiiAssetDocumentTypeDescriptor* type, const xiiPlatformProfile* profile)
{
  if (profile != nullptr && profile != xiiAssetCurator::GetSingleton()->GetActiveAssetProfile())
    s_NonActiveOutputChecks.Increment();
  return xiiAssetDocumentManager::IsOutputUpToDate(path, tag, hash, type, profile);
}

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(ResaveTestProperties, 1, xiiRTTIDefaultAllocator<ResaveTestProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Transform", m_sTransform)->AddAttributes(new xiiAssetBrowserAttribute("ResaveTest", xiiDependencyFlags::Transform)),
    XII_MEMBER_PROPERTY("Thumbnail", m_sThumbnail)->AddAttributes(new xiiAssetBrowserAttribute("ResaveTest", xiiDependencyFlags::Thumbnail)),
    XII_MEMBER_PROPERTY("Package", m_sPackage)->AddAttributes(new xiiAssetBrowserAttribute("ResaveTest", xiiDependencyFlags::Package)),
    XII_MEMBER_PROPERTY("Value", m_sValue),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(ResaveTestDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(ResaveTestManager, 1, xiiRTTIDefaultAllocator<ResaveTestManager>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFrameworkTest/AssetProfileTest.h>
#include <EditorFrameworkTest/AssetTestDocument.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <SharedPluginAssets/TextureAsset/TextureAssetProfile.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

void TestAssetProfiles(xiiQtEditorApp& editor, xiiStringView projectDirectory)
{
  auto*      curator       = xiiAssetCurator::GetSingleton();
  const auto originalIndex = curator->GetActiveAssetProfileIndex();
  auto*      original      = curator->GetAssetProfile(originalIndex);
  auto*      texture       = original->GetTypeConfig<xiiTextureAssetProfileConfig>();
  if (!XII_TEST_BOOL(texture != nullptr))
    return;
  const auto originalResolution = texture->m_uiRenderTargetResolution1;
  auto*      other              = curator->CreateAssetProfile();
  other->SetConfigName("ProfileTest");
  other->AddMissingConfigs();
  other->GetTypeConfig<xiiTextureAssetProfileConfig>()->m_uiRenderTargetResolution1 = 768;
  const auto otherIndex                                                             = curator->GetNumAssetProfiles() - 1;
  XII_TEST_RESULT(curator->SaveRuntimeProfiles());
  XII_SCOPE_EXIT(texture->m_uiRenderTargetResolution1 = originalResolution; XII_TEST_RESULT(curator->SaveRuntimeProfiles()); curator->SetActiveAssetProfileByIndex(originalIndex, true); XII_TEST_RESULT(curator->DeleteAssetProfile(other)););

  xiiStringBuilder path(projectDirectory);
  path.AppendPath("ProfileTarget.xiiRenderTargetAsset");
  auto* asset = xiiDynamicCast<xiiAssetDocument*>(editor.CreateDocument(path, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(asset != nullptr))
    return;
  XII_SCOPE_EXIT(asset->GetDocumentManager()->CloseDocument(asset));
  const auto* properties = asset->GetObjectManager()->GetRootObject()->GetChildren()[0];
  auto*       accessor   = asset->GetObjectAccessor();
  accessor->StartTransaction("Use profile-sized native render target");
  XII_TEST_STATUS(accessor->SetValueByName(properties, "Resolution", static_cast<xiiInt64>(xiiTexture2DResolution::ProfileResolution1)));
  accessor->FinishTransaction();
  XII_TEST_STATUS(asset->SaveDocument());
  const auto* type      = asset->GetAssetDocumentTypeDescriptor();
  auto*       manager   = asset->GetAssetDocumentManager();
  auto        transform = [&](const xiiPlatformProfile* profile) {
    const auto status = curator->TransformAsset(asset->GetGuid(), xiiTransformFlags::TriggeredManually, profile);
    return XII_TEST_BOOL_MSG(status.Succeeded(), "%s", status.m_sMessage.GetData());
  };
  auto read = [&](const xiiPlatformProfile* profile, xiiUInt32 resolution, xiiUInt64 expectedHash) {
    xiiFileReader reader;
    const auto    output = manager->GetAbsoluteOutputFileName(type, path, "", profile);
    if (!XII_TEST_RESULT(reader.Open(output)))
      return false;
    xiiAssetFileHeader header;
    if (!XII_TEST_RESULT(header.Read(reader)))
      return false;
    xiiGALTextureCreationDescription description;
    reader >> description;
    XII_TEST_BOOL(header.GetFileHash() == expectedHash);
    XII_TEST_INT(description.m_Size.width, resolution);
    XII_TEST_INT(description.m_Size.height, resolution);
    return true;
  };
  xiiUInt64 hashA = 0, thumb = 0, package = 0;
  curator->IsAssetUpToDate(asset->GetGuid(), original, type, hashA, thumb, package);
  if (!transform(original) || !read(original, originalResolution, hashA))
    return;
  // Force reevaluation after editing the active profile, as the real profile dialog does.
  texture->m_uiRenderTargetResolution1 = 512;
  XII_TEST_RESULT(curator->SaveRuntimeProfiles());
  curator->SetActiveAssetProfileByIndex(originalIndex, true);
  xiiUInt64  editedHash  = 0;
  const auto editedState = curator->IsAssetUpToDate(asset->GetGuid(), original, type, editedHash, thumb, package);
  // Waiting for curator work during the switch can also finish the pending auto-transform.
  XII_TEST_BOOL(editedState == xiiAssetInfo::NeedsTransform || editedState == xiiAssetInfo::UpToDate);
  XII_TEST_BOOL(editedHash != hashA);
  if (!transform(original) || !read(original, 512, editedHash))
    return;
  xiiStringBuilder dependentPath(projectDirectory);
  dependentPath.AppendPath("ProfileDependent.xiiResaveTest");
  auto* dependent = xiiDynamicCast<ResaveTestDocument*>(editor.CreateDocument(dependentPath, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(dependent != nullptr))
    return;
  XII_SCOPE_EXIT(dependent->GetDocumentManager()->CloseDocument(dependent));
  xiiStringBuilder targetId;
  xiiConversionUtils::ToString(asset->GetGuid(), targetId);
  auto* dependentAccessor = dependent->GetObjectAccessor();
  dependentAccessor->StartTransaction("Reference a profile-specific package dependency");
  XII_TEST_STATUS(dependentAccessor->SetValueByName(dependent->GetPropertyObject(), "Package", targetId.GetView()));
  dependentAccessor->FinishTransaction();
  XII_TEST_STATUS(dependent->SaveDocument());
  xiiUInt64 dependentA = 0, packageA = 0, dependentB = 0, packageB = 0;
  curator->IsAssetUpToDate(dependent->GetGuid(), original, dependent->GetAssetDocumentTypeDescriptor(), dependentA, thumb, packageA);
  curator->IsAssetUpToDate(dependent->GetGuid(), other, dependent->GetAssetDocumentTypeDescriptor(), dependentB, thumb, packageB);
  XII_TEST_BOOL(dependentA == dependentB);
  XII_TEST_BOOL(packageA != packageB);
  curator->IsAssetUpToDate(dependent->GetGuid(), original, dependent->GetAssetDocumentTypeDescriptor(), dependentB, thumb, packageB);
  XII_TEST_BOOL(dependentA == dependentB && packageA == packageB);
  ResaveTestDocument* diamonds[4] = {};
  XII_SCOPE_EXIT(for (auto* diamond : diamonds) if (diamond != nullptr) diamond->GetDocumentManager()->CloseDocument(diamond));
  xiiUuid previous = dependent->GetGuid();
  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    xiiStringBuilder diamondPath(projectDirectory), reference;
    diamondPath.AppendFormat("/ProfileDiamond{}.xiiResaveTest", i);
    diamonds[i] = xiiDynamicCast<ResaveTestDocument*>(editor.CreateDocument(diamondPath, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(diamonds[i] != nullptr))
      return;
    xiiConversionUtils::ToString(previous, reference);
    auto* diamondAccessor = diamonds[i]->GetObjectAccessor();
    diamondAccessor->StartTransaction("Share a dependency across categories");
    XII_TEST_STATUS(diamondAccessor->SetValueByName(diamonds[i]->GetPropertyObject(), "Package", reference.GetView()));
    XII_TEST_STATUS(diamondAccessor->SetValueByName(diamonds[i]->GetPropertyObject(), "Thumbnail", reference.GetView()));
    diamondAccessor->FinishTransaction();
    XII_TEST_STATUS(diamonds[i]->SaveDocument());
    previous = diamonds[i]->GetGuid();
  }
  for (bool force : {false, true})
  {
    ResaveTestManager::s_NonActiveOutputChecks = 0;
    curator->IsAssetUpToDate(previous, other, diamonds[3]->GetAssetDocumentTypeDescriptor(), dependentB, thumb, packageB, force);
    XII_TEST_INT(static_cast<xiiInt32>(ResaveTestManager::s_NonActiveOutputChecks), 5);
  }
  // Explicit target queries and transforms must not silently use or overwrite the active profile.
  xiiUInt64 hashB = 0;
  XII_TEST_BOOL(curator->IsAssetUpToDate(asset->GetGuid(), other, type, hashB, thumb, package) == xiiAssetInfo::NeedsTransform);
  XII_TEST_BOOL(hashB != editedHash);
  if (!transform(other) || !read(other, 768, hashB))
    return;
  xiiUInt64 activeHash = 0;
  XII_TEST_BOOL(curator->IsAssetUpToDate(asset->GetGuid(), original, type, activeHash, thumb, package) == xiiAssetInfo::UpToDate);
  XII_TEST_BOOL(activeHash == editedHash);
  xiiStringBuilder cyclePath(projectDirectory);
  cyclePath.AppendPath("ProfileCycle.xiiResaveTest");
  auto* cycle = xiiDynamicCast<ResaveTestDocument*>(editor.CreateDocument(cyclePath, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(cycle != nullptr))
    return;
  XII_SCOPE_EXIT(cycle->GetDocumentManager()->CloseDocument(cycle));
  xiiStringBuilder self;
  xiiConversionUtils::ToString(cycle->GetGuid(), self);
  auto* cycleAccessor = cycle->GetObjectAccessor();
  cycleAccessor->StartTransaction("Create a cyclic profile query");
  XII_TEST_STATUS(cycleAccessor->SetValueByName(cycle->GetPropertyObject(), "Package", self.GetView()));
  cycleAccessor->FinishTransaction();
  XII_TEST_STATUS(cycle->SaveDocument());
  const auto* cycleType = cycle->GetAssetDocumentTypeDescriptor();
  XII_TEST_BOOL(curator->IsAssetUpToDate(cycle->GetGuid(), original, cycleType, activeHash, thumb, package) == xiiAssetInfo::CircularDependency);
  XII_TEST_BOOL(curator->IsAssetUpToDate(cycle->GetGuid(), original, cycleType, activeHash, thumb, package, true) == xiiAssetInfo::CircularDependency);
  XII_TEST_BOOL(curator->IsAssetUpToDate(cycle->GetGuid(), other, cycleType, activeHash, thumb, package, true) == xiiAssetInfo::CircularDependency);
  XII_TEST_BOOL(activeHash == 0 && thumb == 0 && package == 0);
  XII_TEST_INT(curator->GetActiveAssetProfileIndex(), originalIndex);
  curator->SetActiveAssetProfileByIndex(otherIndex);
  XII_TEST_BOOL(curator->IsAssetUpToDate(asset->GetGuid(), other, type, activeHash, thumb, package) == xiiAssetInfo::UpToDate);
  XII_TEST_BOOL(activeHash == hashB);
  curator->SetActiveAssetProfileByIndex(originalIndex);
  XII_TEST_BOOL(curator->IsAssetUpToDate(asset->GetGuid(), original, type, activeHash, thumb, package) == xiiAssetInfo::UpToDate);
  XII_TEST_BOOL(activeHash == editedHash);
}

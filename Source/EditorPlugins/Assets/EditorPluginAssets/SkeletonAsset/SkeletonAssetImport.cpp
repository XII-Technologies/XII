/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <Foundation/Utilities/Progress.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <ModelImporter/SkeletonImport.h>

xiiTransformStatus xiiSkeletonAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  {
    m_bIsTransforming = true;
    XII_SCOPE_EXIT(m_bIsTransforming = false);

    xiiProgressRange range("Transforming Asset", 3, false);

    xiiEditableSkeleton* pProp = GetProperties();

    xiiStringBuilder sAbsFilename = pProp->m_sSourceFile;

    if (sAbsFilename.IsEmpty())
    {
      range.BeginNextStep("Writing Result");
      XII_SUCCEED_OR_RETURN(WriteResource(stream, *GetProperties()));
    }
    else
    {
      if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
      {
        return xiiStatus(xiiFmt("Couldn't make path absolute: '{}'.", sAbsFilename));
      }

      range.BeginNextStep("Importing Source File");

      xiiModelSkeletonImport imported;
      xiiStringBuilder       error;
      if (xiiModelSkeletonImport::ReadFile(sAbsFilename, imported, &error).Failed())
        return xiiStatus(xiiFmt("Skeleton import failed: {}", error));
      xiiEditableSkeleton                        newSkeleton;
      xiiDynamicArray<xiiEditableSkeletonJoint*> joints;
      joints.Reserve(imported.m_Skeleton.m_Joints.GetCount());
      for (const auto& native : imported.m_Skeleton.m_Joints)
      {
        xiiEditableSkeletonJoint* joint = XII_DEFAULT_NEW(xiiEditableSkeletonJoint);
        joint->m_sName                  = native.m_sName;
        joint->m_LocalTransform         = native.m_LocalRestPose;
        if (native.m_uiParentIndex < joints.GetCount())
          joints[native.m_uiParentIndex]->m_Children.PushBack(joint);
        else
          newSkeleton.m_Children.PushBack(joint);
        joints.PushBack(joint);
      }
      for (auto& collider : imported.m_Colliders)
      {
        auto& authored             = joints[collider.m_uiJointIndex]->m_BoneColliders.ExpandAndGetRef();
        authored.m_sIdentifier     = collider.m_sIdentifier;
        authored.m_VertexPositions = std::move(collider.m_Vertices);
        authored.m_TriangleIndices = std::move(collider.m_TriangleIndices);
      }

      range.BeginNextStep("Importing Skeleton Data");

      // synchronize the old data (collision geometry etc.) with the new hierarchy
      if (GetProperties()->MergeImportedHierarchy(newSkeleton).Failed())
        return xiiStatus("Cannot merge the imported skeleton hierarchy with the document.");

      range.BeginNextStep("Writing Result");
      XII_SUCCEED_OR_RETURN(WriteResource(stream, *GetProperties()));

      // merge the new data with the actual asset document
      const xiiUInt64 beforeImport = GetDocumentHash();
      ApplyNativePropertyChangesToObjectManager(true);
      if (transformFlags.IsSet(xiiTransformFlags::BackgroundProcessing) && beforeImport != GetDocumentHash())
        return xiiTransformStatus(xiiTransformResult::NeedsImport);
    }
  }

  xiiSkeletonAssetEvent e;
  e.m_pDocument = this;
  e.m_Type      = xiiSkeletonAssetEvent::Transformed;
  m_Events.Broadcast(e);

  return XII_SUCCESS;
}

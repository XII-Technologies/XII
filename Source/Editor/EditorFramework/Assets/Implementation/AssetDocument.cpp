#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GuiFoundation/PropertyGrid/PrefabDefaultStateProvider.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <Texture/Image/ImageConversion.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAssetDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAssetDocument::xiiAssetDocument(xiiStringView sDocumentPath, xiiDocumentObjectManager* pObjectManager, xiiAssetDocEngineConnection engineConnectionType) :
  xiiDocument(sDocumentPath, pObjectManager)
{
  m_EngineConnectionType    = engineConnectionType;
  m_EngineStatus            = (m_EngineConnectionType != xiiAssetDocEngineConnection::None) ? EngineStatus::Disconnected : EngineStatus::Unsupported;
  m_pEngineConnection       = nullptr;
  m_uiCommonAssetStateFlags = xiiCommonAssetUiState::Grid | xiiCommonAssetUiState::Loop | xiiCommonAssetUiState::Visualizers;

  if (m_EngineConnectionType != xiiAssetDocEngineConnection::None)
  {
    xiiEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(xiiMakeDelegate(&xiiAssetDocument::EngineConnectionEventHandler, this));
  }
}

xiiAssetDocument::~xiiAssetDocument()
{
  m_pMirror->DeInit();

  if (m_EngineConnectionType != xiiAssetDocEngineConnection::None)
  {
    xiiEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiAssetDocument::EngineConnectionEventHandler, this));

    if (m_pEngineConnection)
    {
      xiiEditorEngineProcessConnection::GetSingleton()->DestroyEngineConnection(this);
    }
  }
}

void xiiAssetDocument::SetCommonAssetUiState(xiiCommonAssetUiState::Enum state, double value)
{
  if (value == 0)
  {
    m_uiCommonAssetStateFlags &= ~((xiiUInt32)state);
  }
  else
  {
    m_uiCommonAssetStateFlags |= (xiiUInt32)state;
  }

  xiiCommonAssetUiState e;
  e.m_State  = state;
  e.m_fValue = value;

  m_CommonAssetUiChangeEvent.Broadcast(e);
}

double xiiAssetDocument::GetCommonAssetUiState(xiiCommonAssetUiState::Enum state) const
{
  return (m_uiCommonAssetStateFlags & (xiiUInt32)state) != 0 ? 1.0f : 0.0f;
}

xiiAssetDocumentManager* xiiAssetDocument::GetAssetDocumentManager() const
{
  return static_cast<xiiAssetDocumentManager*>(GetDocumentManager());
}

const xiiAssetDocumentInfo* xiiAssetDocument::GetAssetDocumentInfo() const
{
  return static_cast<xiiAssetDocumentInfo*>(m_pDocumentInfo);
}

xiiBitflags<xiiAssetDocumentFlags> xiiAssetDocument::GetAssetFlags() const
{
  return GetAssetDocumentTypeDescriptor()->m_AssetDocumentFlags;
}

xiiDocumentInfo* xiiAssetDocument::CreateDocumentInfo()
{
  return XII_DEFAULT_NEW(xiiAssetDocumentInfo);
}

xiiTaskGroupID xiiAssetDocument::InternalSaveDocument(AfterSaveCallback callback)
{
  xiiAssetDocumentInfo* pInfo = static_cast<xiiAssetDocumentInfo*>(m_pDocumentInfo);

  pInfo->m_TransformDependencies.Clear();
  pInfo->m_ThumbnailDependencies.Clear();
  pInfo->m_PackageDependencies.Clear();
  pInfo->m_Outputs.Clear();
  pInfo->m_uiSettingsHash = GetDocumentHash();
  pInfo->m_sAssetsDocumentTypeName.Assign(GetDocumentTypeName());
  pInfo->ClearMetaData();
  UpdateAssetDocumentInfo(pInfo);

  // In case someone added an empty reference.
  pInfo->m_TransformDependencies.Remove(xiiString());
  pInfo->m_ThumbnailDependencies.Remove(xiiString());
  pInfo->m_PackageDependencies.Remove(xiiString());

  return xiiDocument::InternalSaveDocument(callback);
}

void xiiAssetDocument::InternalAfterSaveDocument()
{
  const auto flags = GetAssetFlags();
  xiiAssetCurator::GetSingleton()->NotifyOfFileChange(GetDocumentPath());
  xiiAssetCurator::GetSingleton()->MainThreadTick(false);

  if (flags.IsAnySet(xiiAssetDocumentFlags::AutoTransformOnSave))
  {
    // If we request an engine connection but the mirror is not set up yet we are still
    // creating the document and TransformAsset will most likely fail.
    if (m_EngineConnectionType == xiiAssetDocEngineConnection::None || m_pEngineConnection)
    {
      xiiUuid docGuid = GetGuid();

      xiiSharedPtr<xiiDelegateTask<void>> pTask = XII_DEFAULT_NEW(xiiDelegateTask<void>, "TransformAfterSaveDocument", xiiTaskNesting::Never, [docGuid]() {
        xiiDocument* pDoc = xiiDocumentManager::GetDocumentByGuid(docGuid);
        if (pDoc == nullptr)
          return;

        /// \todo Should only be done for platform agnostic assets
        xiiTransformStatus ret = xiiAssetCurator::GetSingleton()->TransformAsset(docGuid, xiiTransformFlags::TriggeredManually);

        if (ret.Failed())
        {
          xiiLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, pDoc->GetDocumentPath());
        }
        else
        {
          xiiAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
        }
        //
      });

      pTask->ConfigureTask("TransformAfterSaveDocument", xiiTaskNesting::Maybe);
      xiiTaskSystem::StartSingleTask(pTask, xiiTaskPriority::ThisFrameMainThread);
    }
  }
}

void xiiAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  m_pMirror = XII_DEFAULT_NEW(xiiIPCObjectMirrorEditor);
}

void xiiAssetDocument::InitializeAfterLoadingAndSaving()
{
  if (m_EngineConnectionType != xiiAssetDocEngineConnection::None)
  {
    m_pEngineConnection = xiiEditorEngineProcessConnection::GetSingleton()->CreateEngineConnection(this);
    m_EngineStatus      = EngineStatus::Initializing;

    if (m_EngineConnectionType == xiiAssetDocEngineConnection::FullObjectMirroring)
    {
      m_pMirror->SetIPC(m_pEngineConnection);
      m_pMirror->InitSender(GetObjectManager());
    }
  }
}

void xiiAssetDocument::AddPrefabDependencies(const xiiDocumentObject* pObject, xiiAssetDocumentInfo* pInfo) const
{
  {
    const xiiDocumentObjectMetaData* pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());

    if (pMeta->m_CreateFromPrefab.IsValid())
    {
      xiiStringBuilder tmp;
      pInfo->m_TransformDependencies.Insert(xiiConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
    }

    m_DocumentObjectMetaData->EndReadMetaData();
  }


  const xiiHybridArray<xiiDocumentObject*, 8>& children = pObject->GetChildren();

  for (auto pChild : children)
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<xiiTemporaryAttribute>() != nullptr)
      continue;
    AddPrefabDependencies(pChild, pInfo);
  }
}


void xiiAssetDocument::AddReferences(const xiiDocumentObject* pObject, xiiAssetDocumentInfo* pInfo, bool bInsidePrefab) const
{
  {
    const xiiDocumentObjectMetaData* pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());

    if (pMeta->m_CreateFromPrefab.IsValid())
    {
      bInsidePrefab = true;
      xiiStringBuilder tmp;
      pInfo->m_TransformDependencies.Insert(xiiConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
      pInfo->m_ThumbnailDependencies.Insert(xiiConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
    }

    m_DocumentObjectMetaData->EndReadMetaData();
  }

  const xiiRTTI*                                 pType = pObject->GetTypeAccessor().GetType();
  xiiHybridArray<const xiiAbstractProperty*, 32> Properties;
  pType->GetAllProperties(Properties);
  for (auto pProp : Properties)
  {
    if (pProp->GetAttributeByType<xiiTemporaryAttribute>() != nullptr)
      continue;

    xiiBitflags<xiiDependencyFlags> depFlags;

    if (auto pAttr = pProp->GetAttributeByType<xiiAssetBrowserAttribute>())
    {
      depFlags |= pAttr->GetDependencyFlags();
    }

    if (auto pAttr = pProp->GetAttributeByType<xiiFileBrowserAttribute>())
    {
      depFlags |= pAttr->GetDependencyFlags();
    }

    // add all strings that are marked as asset references or file references
    if (depFlags != 0)
    {
      switch (pProp->GetCategory())
      {
        case xiiPropertyCategory::Member:
        {
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::StandardType) && (pProp->GetSpecificType()->GetVariantType() == xiiVariantType::String || pProp->GetSpecificType()->GetVariantType() == xiiVariantType::StringView))
          {
            if (bInsidePrefab)
            {
              xiiHybridArray<xiiPropertySelection, 1> selection;
              selection.PushBack({pObject, xiiVariant()});
              xiiDefaultObjectState defaultState(GetObjectAccessor(), selection.GetArrayPtr());
              if (defaultState.GetStateProviderName() == "Prefab" && defaultState.IsDefaultValue(pProp))
                continue;
            }

            const xiiVariant& value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName());

            if (value.IsA<xiiString>())
            {
              if (depFlags.IsSet(xiiDependencyFlags::Transform))
                pInfo->m_TransformDependencies.Insert(value.Get<xiiString>());

              if (depFlags.IsSet(xiiDependencyFlags::Thumbnail))
                pInfo->m_ThumbnailDependencies.Insert(value.Get<xiiString>());

              if (depFlags.IsSet(xiiDependencyFlags::Package))
                pInfo->m_PackageDependencies.Insert(value.Get<xiiString>());
            }
            else if (value.IsA<xiiStringView>())
            {
              if (depFlags.IsSet(xiiDependencyFlags::Transform))
                pInfo->m_TransformDependencies.Insert(value.Get<xiiStringView>());

              if (depFlags.IsSet(xiiDependencyFlags::Thumbnail))
                pInfo->m_ThumbnailDependencies.Insert(value.Get<xiiStringView>());

              if (depFlags.IsSet(xiiDependencyFlags::Package))
                pInfo->m_PackageDependencies.Insert(value.Get<xiiStringView>());
            }
          }
        }
        break;

        case xiiPropertyCategory::Array:
        case xiiPropertyCategory::Set:
        {
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::StandardType) && (pProp->GetSpecificType()->GetVariantType() == xiiVariantType::String || pProp->GetSpecificType()->GetVariantType() == xiiVariantType::StringView))
          {
            const xiiInt32 iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());

            if (bInsidePrefab)
            {
              xiiHybridArray<xiiPropertySelection, 1> selection;
              selection.PushBack({pObject, xiiVariant()});
              xiiDefaultContainerState defaultState(GetObjectAccessor(), selection.GetArrayPtr(), pProp->GetPropertyName());
              for (xiiInt32 i = 0; i < iCount; ++i)
              {
                xiiVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i);
                if (defaultState.GetStateProviderName() == "Prefab" && defaultState.IsDefaultElement(i))
                {
                  continue;
                }

                if (value.IsA<xiiStringView>())
                {
                  if (depFlags.IsSet(xiiDependencyFlags::Transform))
                    pInfo->m_TransformDependencies.Insert(value.Get<xiiStringView>());

                  if (depFlags.IsSet(xiiDependencyFlags::Thumbnail))
                    pInfo->m_ThumbnailDependencies.Insert(value.Get<xiiStringView>());

                  if (depFlags.IsSet(xiiDependencyFlags::Package))
                    pInfo->m_PackageDependencies.Insert(value.Get<xiiStringView>());
                }
                else
                {
                  if (depFlags.IsSet(xiiDependencyFlags::Transform))
                    pInfo->m_TransformDependencies.Insert(value.Get<xiiString>());

                  if (depFlags.IsSet(xiiDependencyFlags::Thumbnail))
                    pInfo->m_ThumbnailDependencies.Insert(value.Get<xiiString>());

                  if (depFlags.IsSet(xiiDependencyFlags::Package))
                    pInfo->m_PackageDependencies.Insert(value.Get<xiiString>());
                }
              }
            }
            else
            {
              for (xiiInt32 i = 0; i < iCount; ++i)
              {
                xiiVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i);

                if (value.IsA<xiiStringView>())
                {
                  if (depFlags.IsSet(xiiDependencyFlags::Transform))
                    pInfo->m_TransformDependencies.Insert(value.Get<xiiStringView>());

                  if (depFlags.IsSet(xiiDependencyFlags::Thumbnail))
                    pInfo->m_ThumbnailDependencies.Insert(value.Get<xiiStringView>());

                  if (depFlags.IsSet(xiiDependencyFlags::Package))
                    pInfo->m_PackageDependencies.Insert(value.Get<xiiStringView>());
                }
                else
                {
                  if (depFlags.IsSet(xiiDependencyFlags::Transform))
                    pInfo->m_TransformDependencies.Insert(value.Get<xiiString>());

                  if (depFlags.IsSet(xiiDependencyFlags::Thumbnail))
                    pInfo->m_ThumbnailDependencies.Insert(value.Get<xiiString>());

                  if (depFlags.IsSet(xiiDependencyFlags::Package))
                    pInfo->m_PackageDependencies.Insert(value.Get<xiiString>());
                }
              }
            }
          }
        }
        break;

        case xiiPropertyCategory::Map:
          // #TODO Search for exposed params that reference assets.
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::StandardType) && (pProp->GetSpecificType()->GetVariantType() == xiiVariantType::String || pProp->GetSpecificType()->GetVariantType() == xiiVariantType::StringView))
          {
            xiiVariant                  value   = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName());
            const xiiVariantDictionary& varDict = value.Get<xiiVariantDictionary>();
            if (bInsidePrefab)
            {
              xiiHybridArray<xiiPropertySelection, 1> selection;
              selection.PushBack({pObject, xiiVariant()});
              xiiDefaultContainerState defaultState(GetObjectAccessor(), selection.GetArrayPtr(), pProp->GetPropertyName());
              for (auto it : varDict)
              {
                if (defaultState.GetStateProviderName() == "Prefab" && defaultState.IsDefaultElement(it.Key()))
                {
                  continue;
                }

                if (value.IsA<xiiStringView>())
                {
                  if (depFlags.IsSet(xiiDependencyFlags::Transform))
                    pInfo->m_TransformDependencies.Insert(it.Value().Get<xiiStringView>());

                  if (depFlags.IsSet(xiiDependencyFlags::Thumbnail))
                    pInfo->m_ThumbnailDependencies.Insert(it.Value().Get<xiiStringView>());

                  if (depFlags.IsSet(xiiDependencyFlags::Package))
                    pInfo->m_PackageDependencies.Insert(it.Value().Get<xiiStringView>());
                }
                else
                {
                  if (depFlags.IsSet(xiiDependencyFlags::Transform))
                    pInfo->m_TransformDependencies.Insert(it.Value().Get<xiiString>());

                  if (depFlags.IsSet(xiiDependencyFlags::Thumbnail))
                    pInfo->m_ThumbnailDependencies.Insert(it.Value().Get<xiiString>());

                  if (depFlags.IsSet(xiiDependencyFlags::Package))
                    pInfo->m_PackageDependencies.Insert(it.Value().Get<xiiString>());
                }
              }
            }
            else
            {
              for (auto it : varDict)
              {
                if (value.IsA<xiiStringView>())
                {
                  if (depFlags.IsSet(xiiDependencyFlags::Transform))
                    pInfo->m_TransformDependencies.Insert(it.Value().Get<xiiStringView>());

                  if (depFlags.IsSet(xiiDependencyFlags::Thumbnail))
                    pInfo->m_ThumbnailDependencies.Insert(it.Value().Get<xiiStringView>());

                  if (depFlags.IsSet(xiiDependencyFlags::Package))
                    pInfo->m_PackageDependencies.Insert(it.Value().Get<xiiStringView>());
                }
                else
                {
                  if (depFlags.IsSet(xiiDependencyFlags::Transform))
                    pInfo->m_TransformDependencies.Insert(it.Value().Get<xiiString>());

                  if (depFlags.IsSet(xiiDependencyFlags::Thumbnail))
                    pInfo->m_ThumbnailDependencies.Insert(it.Value().Get<xiiString>());

                  if (depFlags.IsSet(xiiDependencyFlags::Package))
                    pInfo->m_PackageDependencies.Insert(it.Value().Get<xiiString>());
                }
              }
            }
          }
          break;

        default:
          break;
      }
    }
  }

  const xiiHybridArray<xiiDocumentObject*, 8>& children = pObject->GetChildren();

  for (auto pChild : children)
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<xiiTemporaryAttribute>() != nullptr)
      continue;

    AddReferences(pChild, pInfo, bInsidePrefab);
  }
}

void xiiAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  const xiiDocumentObject* pRoot = GetObjectManager()->GetRootObject();

  AddPrefabDependencies(pRoot, pInfo);
  AddReferences(pRoot, pInfo, false);
}

void xiiAssetDocument::EngineConnectionEventHandler(const xiiEditorEngineProcessConnection::Event& e)
{
  if (e.m_Type == xiiEditorEngineProcessConnection::Event::Type::ProcessCrashed)
  {
    m_EngineStatus = EngineStatus::Disconnected;
  }
  else if (e.m_Type == xiiEditorEngineProcessConnection::Event::Type::ProcessStarted)
  {
    m_EngineStatus = EngineStatus::Initializing;
  }
}

xiiUInt64 xiiAssetDocument::GetDocumentHash() const
{
  xiiUInt64 uiHash = xiiHashingUtils::xxHash64(&m_pDocumentInfo->m_DocumentID, sizeof(xiiUuid));
  for (auto pChild : GetObjectManager()->GetRootObject()->GetChildren())
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<xiiTemporaryAttribute>() != nullptr)
      continue;
    GetChildHash(pChild, uiHash);
    InternalGetMetaDataHash(pChild, uiHash);
  }

  // Gather used types, sort by name to make it stable and hash their data
  xiiSet<const xiiRTTI*> types;
  xiiToolsReflectionUtils::GatherObjectTypes(GetObjectManager()->GetRootObject(), types);
  xiiDynamicArray<const xiiRTTI*> typesSorted;
  typesSorted.Reserve(types.GetCount());
  for (const xiiRTTI* pType : types)
  {
    typesSorted.PushBack(pType);
  }

  typesSorted.Sort([](const xiiRTTI* a, const xiiRTTI* b) { return a->GetTypeName().Compare(b->GetTypeName()) < 0; });

  for (const xiiRTTI* pType : typesSorted)
  {
    uiHash                 = xiiHashingUtils::xxHash64(pType->GetTypeName().GetStartPointer(), pType->GetTypeName().GetElementCount(), uiHash);
    const xiiUInt32 uiType = pType->GetTypeVersion();
    uiHash                 = xiiHashingUtils::xxHash64(&uiType, sizeof(uiType), uiHash);
  }
  return uiHash;
}

void xiiAssetDocument::GetChildHash(const xiiDocumentObject* pObject, xiiUInt64& uiHash) const
{
  pObject->ComputeObjectHash(uiHash);

  for (auto pChild : pObject->GetChildren())
  {
    GetChildHash(pChild, uiHash);
  }
}

xiiTransformStatus xiiAssetDocument::DoTransformAsset(const xiiPlatformProfile* pAssetProfile0 /*= nullptr*/, xiiBitflags<xiiTransformFlags> transformFlags)
{
  const auto flags = GetAssetFlags();

  if (flags.IsAnySet(xiiAssetDocumentFlags::DisableTransform))
    return xiiStatus("Asset transform has been disabled on this asset");

  const xiiPlatformProfile* pAssetProfile = xiiAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile0);

  xiiUInt64                    uiHash      = 0;
  xiiUInt64                    uiThumbHash = 0;
  xiiAssetInfo::TransformState state       = xiiAssetCurator::GetSingleton()->IsAssetUpToDate(GetGuid(), pAssetProfile, GetAssetDocumentTypeDescriptor(), uiHash, uiThumbHash);
  if (state == xiiAssetInfo::TransformState::UpToDate && !transformFlags.IsSet(xiiTransformFlags::ForceTransform))
    return xiiStatus(XII_SUCCESS, "Transformed asset is already up to date");

  if (uiHash == 0)
    return xiiStatus("Computing the hash for this asset or any dependency failed");

  // Write resource
  {
    xiiAssetFileHeader AssetHeader;
    AssetHeader.SetFileHashAndVersion(uiHash, GetAssetTypeVersion());
    const auto& outputs = GetAssetDocumentInfo()->m_Outputs;

    auto GenerateOutput = [this, pAssetProfile, &AssetHeader, transformFlags](const char* szOutputTag) -> xiiTransformStatus {
      const xiiString    sTargetFile = GetAssetDocumentManager()->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), szOutputTag, pAssetProfile);
      xiiTransformStatus ret         = InternalTransformAsset(sTargetFile, szOutputTag, pAssetProfile, AssetHeader, transformFlags);

      // if writing failed, make sure the output file does not exist
      if (ret.Failed())
      {
        xiiFileSystem::DeleteFile(sTargetFile);
      }
      xiiAssetCurator::GetSingleton()->NotifyOfFileChange(sTargetFile);
      return ret;
    };

    xiiTransformStatus res;
    for (auto it = outputs.GetIterator(); it.IsValid(); ++it)
    {
      res = GenerateOutput(it.Key());
      if (res.Failed())
        return res;
    }

    res = GenerateOutput("");
    if (res.Failed())
      return res;

    xiiAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
    return res;
  }
}

xiiTransformStatus xiiAssetDocument::TransformAsset(xiiBitflags<xiiTransformFlags> transformFlags, const xiiPlatformProfile* pAssetProfile)
{
  XII_PROFILE_SCOPE("TransformAsset");

  if (!transformFlags.IsSet(xiiTransformFlags::ForceTransform))
  {
    XII_SUCCEED_OR_RETURN(SaveDocument().m_Result);

    const auto assetFlags = GetAssetFlags();

    if (assetFlags.IsSet(xiiAssetDocumentFlags::DisableTransform) || (assetFlags.IsSet(xiiAssetDocumentFlags::OnlyTransformManually) && !transformFlags.IsSet(xiiTransformFlags::TriggeredManually)))
    {
      return xiiStatus(XII_SUCCESS, "Transform is disabled for this asset");
    }
  }

  const xiiTransformStatus res = DoTransformAsset(pAssetProfile, transformFlags);

  if (transformFlags.IsSet(xiiTransformFlags::TriggeredManually))
  {
    SaveDocument().LogFailure();
    xiiAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
  }

  return res;
}

xiiTransformStatus xiiAssetDocument::CreateThumbnail()
{
  xiiUInt64 uiHash      = 0;
  xiiUInt64 uiThumbHash = 0;

  xiiAssetInfo::TransformState state = xiiAssetCurator::GetSingleton()->IsAssetUpToDate(GetGuid(), xiiAssetCurator::GetSingleton()->GetActiveAssetProfile(), GetAssetDocumentTypeDescriptor(), uiHash, uiThumbHash);

  if (state == xiiAssetInfo::TransformState::UpToDate)
    return xiiStatus(XII_SUCCESS, "Transformed asset is already up to date");

  if (uiHash == 0)
    return xiiStatus("Computing the hash for this asset or any dependency failed");

  if (state == xiiAssetInfo::NeedsThumbnail)
  {
    ThumbnailInfo ThumbnailInfo;
    ThumbnailInfo.SetFileHashAndVersion(uiThumbHash, GetAssetTypeVersion());
    xiiTransformStatus res = InternalCreateThumbnail(ThumbnailInfo);

    InvalidateAssetThumbnail();
    xiiAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
    return res;
  }
  return xiiTransformStatus(xiiFmt("Asset state is {}", state));
}

xiiTransformStatus xiiAssetDocument::InternalTransformAsset(const char* szTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  xiiDeferredFileWriter file;
  file.SetOutput(szTargetFile);

  if (AssetHeader.Write(file) == XII_FAILURE)
  {
    file.Discard();
    return xiiTransformStatus("Failed to write asset header");
  }

  xiiTransformStatus res = InternalTransformAsset(file, sOutputTag, pAssetProfile, AssetHeader, transformFlags);
  if (res.m_Result != xiiTransformResult::Success)
  {
    // We do not want to overwrite the old output file if we failed to transform the asset.
    file.Discard();
    return res;
  }

  if (file.Close().Failed())
  {
    xiiLog::Error("Could not open file for writing: '{0}'", szTargetFile);
    return xiiStatus("Opening the asset output file failed");
  }

  return xiiStatus(XII_SUCCESS);
}

xiiString xiiAssetDocument::GetThumbnailFilePath(xiiStringView sSubAssetName /*= xiiStringView()*/) const
{
  return GetAssetDocumentManager()->GenerateResourceThumbnailPath(GetDocumentPath(), sSubAssetName);
}

void xiiAssetDocument::InvalidateAssetThumbnail(xiiStringView sSubAssetName /*= xiiStringView()*/) const
{
  const xiiString sResourceFile = GetThumbnailFilePath(sSubAssetName);
  xiiAssetCurator::GetSingleton()->NotifyOfFileChange(sResourceFile);
  xiiQtImageCache::GetSingleton()->InvalidateCache(sResourceFile);
}

xiiStatus xiiAssetDocument::SaveThumbnail(const xiiImage& img, const ThumbnailInfo& thumbnailInfo) const
{
  xiiImage converted;

  // make sure the thumbnail is in a format that Qt understands

  /// \todo A conversion to B8G8R8X8_UNORM currently fails

  if (xiiImageConversion::Convert(img, converted, xiiImageFormat::R8G8B8A8_UNORM).Failed())
  {
    const xiiStringBuilder sResourceFile = GetThumbnailFilePath();

    xiiLog::Error("Could not convert asset thumbnail to target format: '{0}'", sResourceFile);
    return xiiStatus(xiiFmt("Could not convert asset thumbnail to target format: '{0}'", sResourceFile));
  }

  QImage qimg(converted.GetPixelPointer<xiiUInt8>(), converted.GetWidth(), converted.GetHeight(), QImage::Format_RGBA8888);

  return SaveThumbnail(qimg, thumbnailInfo);
}

xiiStatus xiiAssetDocument::SaveThumbnail(const QImage& qimg0, const ThumbnailInfo& thumbnailInfo) const
{
  const xiiStringBuilder sResourceFile = GetThumbnailFilePath();
  XII_LOG_BLOCK("Save Asset Thumbnail", sResourceFile.GetData());

  QImage qimg = qimg0;

  if (qimg.width() == qimg.height())
  {
    // if necessary scale the image to the proper size
    if (qimg.width() != xiiThumbnailSize)
      qimg = qimg.scaled(xiiThumbnailSize, xiiThumbnailSize, Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation);
  }
  else
  {
    // center the image in a square canvas

    // scale the longer edge to xiiThumbnailSize
    if (qimg.width() > qimg.height())
      qimg = qimg.scaledToWidth(xiiThumbnailSize, Qt::TransformationMode::SmoothTransformation);
    else
      qimg = qimg.scaledToHeight(xiiThumbnailSize, Qt::TransformationMode::SmoothTransformation);

    // create a black canvas
    QImage img2(xiiThumbnailSize, xiiThumbnailSize, QImage::Format_RGBA8888);
    img2.fill(Qt::GlobalColor::black);

    QPoint destPos = QPoint((xiiThumbnailSize - qimg.width()) / 2, (xiiThumbnailSize - qimg.height()) / 2);

    // paint the smaller image such that it ends up centered
    QPainter painter(&img2);
    painter.drawImage(destPos, qimg);
    painter.end();

    qimg = img2;
  }

  // make sure the directory exists, Qt will not create sub-folders
  const xiiStringBuilder sDir = sResourceFile.GetFileDirectory();
  XII_SUCCEED_OR_RETURN(xiiOSFile::CreateDirectoryStructure(sDir));

  // save to JPEG
  if (!qimg.save(QString::fromUtf8(sResourceFile.GetData()), nullptr, 90))
  {
    xiiLog::Error("Could not save asset thumbnail: '{0}'", sResourceFile);
    return xiiStatus(xiiFmt("Could not save asset thumbnail: '{0}'", sResourceFile));
  }

  AppendThumbnailInfo(sResourceFile, thumbnailInfo);
  InvalidateAssetThumbnail();

  return xiiStatus(XII_SUCCESS);
}

void xiiAssetDocument::AppendThumbnailInfo(xiiStringView sThumbnailFile, const ThumbnailInfo& thumbnailInfo) const
{
  xiiContiguousMemoryStreamStorage storage;
  {
    xiiFileReader reader;
    if (reader.Open(sThumbnailFile).Failed())
    {
      return;
    }
    storage.ReadAll(reader);
  }

  xiiDeferredFileWriter writer;
  writer.SetOutput(sThumbnailFile);
  writer.WriteBytes(storage.GetData(), storage.GetStorageSize64()).IgnoreResult();

  thumbnailInfo.Serialize(writer).IgnoreResult();

  if (writer.Close().Failed())
  {
    xiiLog::Error("Could not open file for writing: '{0}'", sThumbnailFile);
  }
}

xiiStatus xiiAssetDocument::RemoteExport(const xiiAssetFileHeader& header, const char* szOutputTarget) const
{
  xiiProgressRange range("Exporting Asset", 2, false);

  xiiLog::Info("Exporting {0} to \"{1}\"", GetDocumentTypeName(), szOutputTarget);

  if (GetEngineStatus() == xiiAssetDocument::EngineStatus::Disconnected)
  {
    return xiiStatus(xiiFmt("Exporting {0} to \"{1}\" failed, engine not started or crashed.", GetDocumentTypeName(), szOutputTarget));
  }
  else if (GetEngineStatus() == xiiAssetDocument::EngineStatus::Initializing)
  {
    if (xiiEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), xiiDocumentOpenResponseMsgToEditor::GetStaticRTTI(), xiiTime::Seconds(10)).Failed())
    {
      return xiiStatus(xiiFmt("Exporting {0} to \"{1}\" failed, document initialization timed out.", GetDocumentTypeName(), szOutputTarget));
    }
    XII_ASSERT_DEV(GetEngineStatus() == xiiAssetDocument::EngineStatus::Loaded, "After receiving xiiDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }

  range.BeginNextStep(szOutputTarget);

  xiiExportDocumentMsgToEngine msg;
  msg.m_sOutputFile = szOutputTarget;
  msg.m_uiAssetHash = header.GetFileHash();
  msg.m_uiVersion   = header.GetFileVersion();

  GetEditorEngineConnection()->SendMessage(&msg);

  xiiStatus                                              status(XII_FAILURE);
  xiiProcessCommunicationChannel::WaitForMessageCallback callback = [&status](xiiProcessMessage* pMsg) -> bool {
    xiiExportDocumentMsgToEditor* pMsg2 = xiiDynamicCast<xiiExportDocumentMsgToEditor*>(pMsg);
    status                              = xiiStatus(pMsg2->m_bOutputSuccess ? XII_SUCCESS : XII_FAILURE, pMsg2->m_sFailureMsg);
    return true;
  };

  if (xiiEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), xiiExportDocumentMsgToEditor::GetStaticRTTI(), xiiTime::Seconds(60), &callback).Failed())
  {
    return xiiStatus(xiiFmt("Remote exporting {0} to \"{1}\" timed out.", GetDocumentTypeName(), msg.m_sOutputFile));
  }
  else
  {
    if (status.Failed())
    {
      return status;
    }

    xiiLog::Success("{0} \"{1}\" has been exported.", GetDocumentTypeName(), msg.m_sOutputFile);

    ShowDocumentStatus(xiiFmt("{0} exported successfully", GetDocumentTypeName()));

    return xiiStatus(XII_SUCCESS);
  }
}

xiiTransformStatus xiiAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& thumbnailInfo)
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return xiiStatus("Not implemented");
}

xiiStatus xiiAssetDocument::RemoteCreateThumbnail(const ThumbnailInfo& thumbnailInfo, xiiArrayPtr<xiiStringView> viewExclusionTags) const
{
  xiiAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();

  xiiLog::Info("Create {0} thumbnail for \"{1}\"", GetDocumentTypeName(), GetDocumentPath());

  if (GetEngineStatus() == xiiAssetDocument::EngineStatus::Disconnected)
  {
    return xiiStatus(xiiFmt("Create {0} thumbnail for \"{1}\" failed, engine not started or crashed.", GetDocumentTypeName(), GetDocumentPath()));
  }
  else if (GetEngineStatus() == xiiAssetDocument::EngineStatus::Initializing)
  {
    if (xiiEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), xiiDocumentOpenResponseMsgToEditor::GetStaticRTTI(), xiiTime::Seconds(10)).Failed())
    {
      return xiiStatus(xiiFmt("Create {0} thumbnail for \"{1}\" failed, document initialization timed out.", GetDocumentTypeName(), GetDocumentPath()));
    }
    XII_ASSERT_DEV(GetEngineStatus() == xiiAssetDocument::EngineStatus::Loaded, "After receiving xiiDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }

  SyncObjectsToEngine();
  xiiCreateThumbnailMsgToEngine msg;
  msg.m_uiWidth  = xiiThumbnailSize;
  msg.m_uiHeight = xiiThumbnailSize;
  for (const xiiStringView& tag : viewExclusionTags)
  {
    msg.m_ViewExcludeTags.PushBack(tag);
  }
  GetEditorEngineConnection()->SendMessage(&msg);

  xiiDataBuffer                                          data;
  xiiProcessCommunicationChannel::WaitForMessageCallback callback = [&data](xiiProcessMessage* pMsg) -> bool {
    xiiCreateThumbnailMsgToEditor* pThumbnailMsg = xiiDynamicCast<xiiCreateThumbnailMsgToEditor*>(pMsg);
    data                                         = pThumbnailMsg->m_ThumbnailData;
    return true;
  };

  if (xiiEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), xiiCreateThumbnailMsgToEditor::GetStaticRTTI(), xiiTime::Seconds(60), &callback).Failed())
  {
    return xiiStatus(xiiFmt("Create {0} thumbnail for \"{1}\" failed timed out.", GetDocumentTypeName(), GetDocumentPath()));
  }
  else
  {
    if (data.GetCount() != msg.m_uiWidth * msg.m_uiHeight * 4)
    {
      return xiiStatus(xiiFmt("Thumbnail generation for {0} failed, thumbnail data is empty.", GetDocumentTypeName()));
    }

    xiiImageHeader imgHeader;
    imgHeader.SetImageFormat(xiiImageFormat::R8G8B8A8_UNORM);
    imgHeader.SetWidth(msg.m_uiWidth);
    imgHeader.SetHeight(msg.m_uiHeight);

    xiiImage image;
    image.ResetAndAlloc(imgHeader);
    XII_ASSERT_DEV(data.GetCount() == imgHeader.ComputeDataSize(), "Thumbnail xiiImage has different size than data buffer!");
    xiiMemoryUtils::Copy(image.GetPixelPointer<xiiUInt8>(), data.GetData(), msg.m_uiWidth * msg.m_uiHeight * 4);
    SaveThumbnail(image, thumbnailInfo).LogFailure();

    xiiLog::Success("{0} thumbnail for \"{1}\" has been exported.", GetDocumentTypeName(), GetDocumentPath());

    ShowDocumentStatus(xiiFmt("{0} thumbnail created successfully", GetDocumentTypeName()));

    return xiiStatus(XII_SUCCESS);
  }
}

xiiUInt16 xiiAssetDocument::GetAssetTypeVersion() const
{
  return (xiiUInt16)GetDynamicRTTI()->GetTypeVersion();
}

bool xiiAssetDocument::SendMessageToEngine(xiiEditorEngineDocumentMsg* pMessage /*= false*/) const
{
  return GetEditorEngineConnection()->SendMessage(pMessage);
}

void xiiAssetDocument::HandleEngineMessage(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiDocumentOpenResponseMsgToEditor>())
  {
    if (m_EngineConnectionType == xiiAssetDocEngineConnection::FullObjectMirroring)
    {
      // make sure the engine clears the document first
      xiiDocumentClearMsgToEngine msgClear;
      msgClear.m_DocumentGuid = GetGuid();
      SendMessageToEngine(&msgClear);

      m_pMirror->SendDocument();
    }
    m_EngineStatus = EngineStatus::Loaded;
    // make sure all sync objects are 'modified' so that they will get resent as well
    for (auto* pObject : m_SyncObjects)
    {
      pObject->SetModified();
    }
  }

  m_ProcessMessageEvent.Broadcast(pMsg);
}

void xiiAssetDocument::AddSyncObject(xiiEditorEngineSyncObject* pSync) const
{
  pSync->Configure(GetGuid(), [this](xiiEditorEngineSyncObject* pSync) { RemoveSyncObject(pSync); });

  m_SyncObjects.PushBack(pSync);
  m_AllSyncObjects[pSync->GetGuid()] = pSync;
}

void xiiAssetDocument::RemoveSyncObject(xiiEditorEngineSyncObject* pSync) const
{
  m_DeletedObjects.PushBack(pSync->GetGuid());
  m_AllSyncObjects.Remove(pSync->GetGuid());
  m_SyncObjects.RemoveAndSwap(pSync);
}

xiiEditorEngineSyncObject* xiiAssetDocument::FindSyncObject(const xiiUuid& guid) const
{
  xiiEditorEngineSyncObject* pSync = nullptr;
  m_AllSyncObjects.TryGetValue(guid, pSync);
  return pSync;
}

xiiEditorEngineSyncObject* xiiAssetDocument::FindSyncObject(const xiiRTTI* pType) const
{
  for (xiiEditorEngineSyncObject* pSync : m_SyncObjects)
  {
    if (pSync->GetDynamicRTTI() == pType)
    {
      return pSync;
    }
  }
  return nullptr;
}

void xiiAssetDocument::SyncObjectsToEngine() const
{
  // Tell the engine which sync objects have been removed recently
  {
    for (const auto& guid : m_DeletedObjects)
    {
      xiiEditorEngineSyncObjectMsg msg;
      msg.m_ObjectGuid = guid;
      SendMessageToEngine(&msg);
    }

    m_DeletedObjects.Clear();
  }

  for (auto* pObject : m_SyncObjects)
  {
    if (!pObject->GetModified())
      continue;

    xiiEditorEngineSyncObjectMsg msg;
    msg.m_ObjectGuid  = pObject->m_SyncObjectGuid;
    msg.m_sObjectType = pObject->GetDynamicRTTI()->GetTypeName();

    xiiContiguousMemoryStreamStorage storage;
    xiiMemoryStreamWriter            writer(&storage);
    xiiMemoryStreamReader            reader(&storage);

    xiiReflectionSerializer::WriteObjectToBinary(writer, pObject->GetDynamicRTTI(), pObject);
    msg.m_ObjectData = xiiArrayPtr<const xiiUInt8>(storage.GetData(), storage.GetStorageSize32());

    SendMessageToEngine(&msg);

    pObject->SetModified(false);
  }
}

void xiiAssetDocument::SendDocumentOpenMessage(bool bOpen)
{
  XII_PROFILE_SCOPE("SendDocumentOpenMessage");

  // it is important to have up-to-date lookup tables in the engine process, because document contexts might try to
  // load resources, and if the file redirection does not happen correctly, derived resource types may not be created as they should
  xiiAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();

  m_EngineStatus = EngineStatus::Initializing;

  xiiDocumentOpenMsgToEngine m;
  m.m_DocumentGuid     = GetGuid();
  m.m_bDocumentOpen    = bOpen;
  m.m_sDocumentType    = GetDocumentTypeDescriptor()->m_sDocumentTypeName;
  m.m_DocumentMetaData = GetCreateEngineMetaData();

  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&m);
}

namespace
{
  static const char* szThumbnailInfoTag = "xiThumb";
}

xiiResult xiiAssetDocument::ThumbnailInfo::Deserialize(xiiStreamReader& inout_reader)
{
  char tag[8] = {0};

  if (inout_reader.ReadBytes(tag, 7) != 7)
    return XII_FAILURE;

  if (!xiiStringUtils::IsEqual(tag, szThumbnailInfoTag))
  {
    return XII_FAILURE;
  }

  inout_reader >> m_uiHash;
  inout_reader >> m_uiVersion;
  inout_reader >> m_uiReserved;

  return XII_SUCCESS;
}

xiiResult xiiAssetDocument::ThumbnailInfo::Serialize(xiiStreamWriter& inout_writer) const
{
  XII_SUCCEED_OR_RETURN(inout_writer.WriteBytes(szThumbnailInfoTag, 7));

  inout_writer << m_uiHash;
  inout_writer << m_uiVersion;
  inout_writer << m_uiReserved;

  return XII_SUCCESS;
}

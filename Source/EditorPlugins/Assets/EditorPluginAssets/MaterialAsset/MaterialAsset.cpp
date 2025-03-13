#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/ShaderTypeRegistry.h>
#include <EditorPluginAssets/VisualShader/VsCodeGenerator.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/ShaderCompiler/ShaderParser.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

namespace
{
  xiiResult AddDefines(xiiDynamicArray<xiiString>& inout_defines, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp)
  {
    xiiStringBuilder sDefine;

    xiiStringView sName = pProp->GetPropertyName();
    if (pProp->GetSpecificType()->GetVariantType() == xiiVariantType::Bool)
    {
      sDefine.Set(sName, " ", pObject->GetTypeAccessor().GetValue(sName).Get<bool>() ? "TRUE" : "FALSE");
      inout_defines.PushBack(sDefine);
      return XII_SUCCESS;
    }
    else if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
    {
      xiiInt64 iValue = pObject->GetTypeAccessor().GetValue(sName).ConvertTo<xiiInt64>();

      xiiHybridArray<xiiReflectionUtils::EnumKeyValuePair, 16> enumValues;
      xiiReflectionUtils::GetEnumKeysAndValues(pProp->GetSpecificType(), enumValues, xiiReflectionUtils::EnumConversionMode::ValueNameOnly);
      for (auto& enumValue : enumValues)
      {
        sDefine.SetFormat("{} {}", enumValue.m_sKey, enumValue.m_iValue);
        inout_defines.PushBack(sDefine);

        if (enumValue.m_iValue == iValue)
        {
          sDefine.Set(sName, " ", enumValue.m_sKey);
          inout_defines.PushBack(sDefine);
        }
      }

      return XII_SUCCESS;
    }

    XII_REPORT_FAILURE("Invalid shader permutation property type '{0}'", pProp->GetSpecificType()->GetTypeName());
    return XII_FAILURE;
  }

  xiiResult ParseMaterialConfig(xiiStringView sRelativeFileName, const xiiDocumentObject* pShaderPropertyObject, xiiVariantDictionary& out_materialConfig)
  {
    xiiFileReader file;
    if (file.Open(sRelativeFileName).Failed())
      return XII_FAILURE;

    xiiHybridArray<xiiString, 16> defines;
    {
      xiiHybridArray<const xiiAbstractProperty*, 32> properties;
      pShaderPropertyObject->GetType()->GetAllProperties(properties);

      for (auto& pProp : properties)
      {
        const xiiCategoryAttribute* pCategory = pProp->GetAttributeByType<xiiCategoryAttribute>();
        if (pCategory == nullptr || pCategory->GetCategory() != "Permutation")
          continue;

        XII_SUCCEED_OR_RETURN(AddDefines(defines, pShaderPropertyObject, pProp));
      }
    }

    xiiStringBuilder sOutput;
    XII_SUCCEED_OR_RETURN(xiiShaderParser::PreprocessSection(file, xiiShaderHelper::xiiShaderSections::MATERIALCONFIG, defines, sOutput));

    xiiHybridArray<xiiStringView, 32> allAssignments;
    sOutput.Split(false, allAssignments, "\n", ";", "\r");

    xiiStringBuilder                 temp;
    xiiHybridArray<xiiStringView, 4> components;
    for (const xiiStringView& assignment : allAssignments)
    {
      temp = assignment;
      temp.Trim(" \t\r\n;");
      if (temp.IsEmpty())
        continue;

      temp.Split(false, components, " ", "\t", "=", "\r");

      if (components.GetCount() != 2)
      {
        xiiLog::Error("Malformed shader state assignment: '{0}'", temp);
        continue;
      }

      out_materialConfig[components[0]] = components[1];
    }

    return XII_SUCCESS;
  }
} // namespace

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMaterialAssetPreview, 1)
  XII_ENUM_CONSTANT(xiiMaterialAssetPreview::Sphere),
  XII_ENUM_CONSTANT(xiiMaterialAssetPreview::Box),
  XII_ENUM_CONSTANT(xiiMaterialAssetPreview::Plane),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMaterialShaderMode, 1)
XII_ENUM_CONSTANTS(xiiMaterialShaderMode::BaseMaterial, xiiMaterialShaderMode::File, xiiMaterialShaderMode::Custom)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMaterialAssetProperties, 4, xiiRTTIDefaultAllocator<xiiMaterialAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("ShaderMode", xiiMaterialShaderMode, GetShaderMode, SetShaderMode),
    XII_ACCESSOR_PROPERTY("BaseMaterial", GetBaseMaterial, SetBaseMaterial)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material", xiiDependencyFlags::Transform | xiiDependencyFlags::Thumbnail | xiiDependencyFlags::Package)),
    XII_ACCESSOR_PROPERTY("Surface", GetSurface, SetSurface)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface", xiiDependencyFlags::Package)),
    XII_MEMBER_PROPERTY("AssetFilterTags", m_sAssetFilterTags),
    XII_ACCESSOR_PROPERTY("Shader", GetShader, SetShader)->AddAttributes(new xiiFileBrowserAttribute("Select Shader", "*.xiiShader", "CustomAction_CreateShaderFromTemplate")),
    // This property holds the phantom shader properties type so it is only used in the object graph but not actually in the instance of this object.
    XII_ACCESSOR_PROPERTY("ShaderProperties", GetShaderProperties, SetShaderProperties)->AddFlags(xiiPropertyFlags::PointerOwner)->AddAttributes(new xiiContainerAttribute(false, false, false)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMaterialAssetDocument, 7, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiUuid xiiMaterialAssetDocument::s_LitBaseMaterial;
xiiUuid xiiMaterialAssetDocument::s_LitAlphaTextBaseMaterial;
xiiUuid xiiMaterialAssetDocument::s_NeutralNormalMap;

void xiiMaterialAssetProperties::SetBaseMaterial(const char* szBaseMaterial)
{
  if (m_sBaseMaterial == szBaseMaterial)
    return;

  m_sBaseMaterial = szBaseMaterial;

  // If no doc is present, we are de-serializing the document so do nothing yet.
  if (!m_pDocument)
    return;
  if (m_pDocument->GetCommandHistory()->IsInUndoRedo())
    return;
  m_pDocument->SetBaseMaterial(m_sBaseMaterial);
}

const char* xiiMaterialAssetProperties::GetBaseMaterial() const
{
  return m_sBaseMaterial;
}

void xiiMaterialAssetProperties::SetShader(const char* szShader)
{
  if (m_sShader != szShader)
  {
    m_sShader = szShader;
    UpdateShader();
  }
}

const char* xiiMaterialAssetProperties::GetShader() const
{
  return m_sShader;
}

void xiiMaterialAssetProperties::SetShaderProperties(xiiReflectedClass* pProperties)
{
  // This property represents the phantom shader type, so it is never actually used.
}

xiiReflectedClass* xiiMaterialAssetProperties::GetShaderProperties() const
{
  // This property represents the phantom shader type, so it is never actually used.
  return nullptr;
}

void xiiMaterialAssetProperties::SetShaderMode(xiiEnum<xiiMaterialShaderMode> mode)
{
  if (m_ShaderMode == mode)
    return;

  m_ShaderMode = mode;

  // If no doc is present, we are de-serializing the document so do nothing yet.
  if (!m_pDocument)
    return;
  xiiCommandHistory*     pHistory  = m_pDocument->GetCommandHistory();
  xiiObjectAccessorBase* pAccessor = m_pDocument->GetObjectAccessor();
  // Do not make new commands if we got here in a response to an undo / redo action.
  if (pHistory->IsInUndoRedo())
    return;

  xiiStringBuilder tmp;

  switch (m_ShaderMode)
  {
    case xiiMaterialShaderMode::BaseMaterial:
    {
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "BaseMaterial", "").AssertSuccess();
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "Shader", "").AssertSuccess();
    }
    break;
    case xiiMaterialShaderMode::File:
    {
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "BaseMaterial", "").AssertSuccess();
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "Shader", "").AssertSuccess();
    }
    break;
    case xiiMaterialShaderMode::Custom:
    {
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "BaseMaterial", "").AssertSuccess();
      pAccessor->SetValueByName(m_pDocument->GetPropertyObject(), "Shader", xiiConversionUtils::ToString(m_pDocument->GetGuid(), tmp).GetData()).AssertSuccess();
    }
    break;
  }
}

void xiiMaterialAssetProperties::SetDocument(xiiMaterialAssetDocument* pDocument)
{
  m_pDocument = pDocument;
  if (!m_sBaseMaterial.IsEmpty())
  {
    m_pDocument->SetBaseMaterial(m_sBaseMaterial);
  }
  UpdateShader(true);
}


void xiiMaterialAssetProperties::UpdateShader(bool bForce)
{
  // If no doc is present, we are de-serializing the document so do nothing yet.
  if (!m_pDocument)
    return;

  xiiCommandHistory* pHistory = m_pDocument->GetCommandHistory();
  // Do not make new commands if we got here in a response to an undo / redo action.
  if (pHistory->IsInUndoRedo())
    return;

  XII_ASSERT_DEBUG(pHistory->IsInTransaction(), "Missing undo scope on stack.");

  xiiDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();

  // TODO: If m_sShader is empty, we need to get the shader of our base material and use that one instead
  // for the code below. The type name is the clean path to the shader at the moment.
  xiiStringBuilder sShaderPath = ResolveRelativeShaderPath();
  sShaderPath.MakeCleanPath();

  if (sShaderPath.IsEmpty())
  {
    // No shader, delete any existing properties object.
    if (pPropObject)
    {
      DeleteProperties();
    }
  }
  else
  {
    if (pPropObject)
    {
      // We already have a shader properties object, test whether
      // it has a different type than the newly set shader. The type name
      // is the clean path to the shader at the moment.
      const xiiRTTI* pType = pPropObject->GetTypeAccessor().GetType();
      if (sShaderPath != pType->GetTypeName() || bForce) // TODO: Is force even necessary anymore?
      {
        // Shader has changed, delete old and create new one.
        DeleteProperties();
        CreateProperties(sShaderPath);
      }
      else
      {
        // Same shader but it could have changed so try to update it anyway.
        xiiShaderTypeRegistry::GetSingleton()->GetShaderType(sShaderPath);
      }
    }

    if (!pPropObject)
    {
      // No shader properties exist yet, so create a new one.
      CreateProperties(sShaderPath);
    }
  }
}

void xiiMaterialAssetProperties::DeleteProperties()
{
  SaveOldValues();
  xiiCommandHistory*     pHistory    = m_pDocument->GetCommandHistory();
  xiiDocumentObject*     pPropObject = m_pDocument->GetShaderPropertyObject();
  xiiRemoveObjectCommand cmd;
  cmd.m_Object = pPropObject->GetGuid();
  auto res     = pHistory->AddCommand(cmd);
  XII_ASSERT_DEV(res.m_Result.Succeeded(), "Removal of old properties should never fail.");
}

void xiiMaterialAssetProperties::CreateProperties(const char* szShaderPath)
{
  xiiCommandHistory* pHistory = m_pDocument->GetCommandHistory();

  const xiiRTTI* pType = xiiShaderTypeRegistry::GetSingleton()->GetShaderType(szShaderPath);
  if (!pType && m_ShaderMode == xiiMaterialShaderMode::Custom)
  {
    // Force generate if custom shader is missing
    xiiAssetFileHeader AssetHeader;
    AssetHeader.SetFileHashAndVersion(0, m_pDocument->GetAssetTypeVersion());
    m_pDocument->RecreateVisualShaderFile(AssetHeader).LogFailure();
    pType = xiiShaderTypeRegistry::GetSingleton()->GetShaderType(szShaderPath);
  }

  if (pType)
  {
    xiiAddObjectCommand cmd;
    cmd.m_pType           = pType;
    cmd.m_sParentProperty = "ShaderProperties";
    cmd.m_Parent          = m_pDocument->GetPropertyObject()->GetGuid();
    cmd.m_NewObjectGuid   = cmd.m_Parent;
    cmd.m_NewObjectGuid.CombineWithSeed(xiiUuid::MakeStableUuidFromString("ShaderProperties"));

    auto res = pHistory->AddCommand(cmd);
    XII_ASSERT_DEV(res.m_Result.Succeeded(), "Addition of new properties should never fail.");
    LoadOldValues();
  }
}

void xiiMaterialAssetProperties::SaveOldValues()
{
  xiiDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();
  if (pPropObject)
  {
    const xiiIReflectedTypeAccessor&               accessor = pPropObject->GetTypeAccessor();
    const xiiRTTI*                                 pType    = accessor.GetType();
    xiiHybridArray<const xiiAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetCategory() == xiiPropertyCategory::Member)
      {
        m_CachedProperties[pProp->GetPropertyName()] = accessor.GetValue(pProp->GetPropertyName());
      }
    }
  }
}

void xiiMaterialAssetProperties::LoadOldValues()
{
  xiiDocumentObject* pPropObject = m_pDocument->GetShaderPropertyObject();
  xiiCommandHistory* pHistory    = m_pDocument->GetCommandHistory();
  if (pPropObject)
  {
    const xiiIReflectedTypeAccessor&               accessor = pPropObject->GetTypeAccessor();
    const xiiRTTI*                                 pType    = accessor.GetType();
    xiiHybridArray<const xiiAbstractProperty*, 32> properties;
    pType->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetCategory() == xiiPropertyCategory::Member)
      {
        xiiString sPropName = pProp->GetPropertyName();
        auto      it        = m_CachedProperties.Find(sPropName);
        if (it.IsValid())
        {
          if (it.Value() != accessor.GetValue(sPropName.GetData()))
          {
            xiiSetObjectPropertyCommand cmd;
            cmd.m_Object    = pPropObject->GetGuid();
            cmd.m_sProperty = sPropName;
            cmd.m_NewValue  = it.Value();

            // Do not check for success, if a cached value failed to apply, simply ignore it.
            pHistory->AddCommand(cmd).AssertSuccess();
          }
        }
      }
    }
  }
}

xiiString xiiMaterialAssetProperties::GetAutoGenShaderPathAbs() const
{
  xiiAssetDocumentManager* pManager       = xiiDynamicCast<xiiAssetDocumentManager*>(m_pDocument->GetDocumentManager());
  xiiString                sAbsOutputPath = pManager->GetAbsoluteOutputFileName(m_pDocument->GetAssetDocumentTypeDescriptor(), m_pDocument->GetDocumentPath(), xiiMaterialAssetDocumentManager::s_szShaderOutputTag);
  return sAbsOutputPath;
}

void xiiMaterialAssetProperties::PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == xiiGetStaticRTTI<xiiMaterialAssetProperties>())
  {
    xiiInt64 shaderMode = e.m_pObject->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<xiiInt64>();

    auto& props = *e.m_pPropertyStates;

    if (shaderMode == xiiMaterialShaderMode::File)
      props["Shader"].m_Visibility = xiiPropertyUiState::Default;
    else
      props["Shader"].m_Visibility = xiiPropertyUiState::Invisible;

    if (shaderMode == xiiMaterialShaderMode::BaseMaterial)
      props["BaseMaterial"].m_Visibility = xiiPropertyUiState::Default;
    else
      props["BaseMaterial"].m_Visibility = xiiPropertyUiState::Invisible;
  }
}

xiiString xiiMaterialAssetProperties::ResolveRelativeShaderPath() const
{
  bool isGuid = xiiConversionUtils::IsStringUuid(m_sShader);

  if (isGuid)
  {
    xiiUuid guid   = xiiConversionUtils::ConvertStringToUuid(m_sShader);
    auto    pAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(guid);
    if (pAsset)
    {
      XII_ASSERT_DEV(pAsset->m_pAssetInfo->GetManager() == m_pDocument->GetDocumentManager(), "Referenced shader via guid by this material is not of type material asset (xiiMaterialShaderMode::Custom).");

      xiiStringBuilder sProjectDir = xiiAssetCurator::GetSingleton()->FindDataDirectoryForAsset(pAsset->m_pAssetInfo->m_Path);
      xiiStringBuilder sResult     = pAsset->m_pAssetInfo->GetManager()->GetRelativeOutputFileName(m_pDocument->GetAssetDocumentTypeDescriptor(), sProjectDir, pAsset->m_pAssetInfo->m_Path, xiiMaterialAssetDocumentManager::s_szShaderOutputTag);

      sResult.Prepend("AssetCache/");
      return sResult;
    }
    else
    {
      xiiLog::Error("Could not resolve guid '{0}' for the material shader.", m_sShader);
      return "";
    }
  }
  else
  {
    return m_sShader;
  }

  return m_sShader;
}

//////////////////////////////////////////////////////////////////////////

xiiMaterialAssetDocument::xiiMaterialAssetDocument(xiiStringView sDocumentPath) :
  xiiSimpleAssetDocument<xiiMaterialAssetProperties>(XII_DEFAULT_NEW(xiiMaterialObjectManager), sDocumentPath, xiiAssetDocEngineConnection::Simple, true)
{
  xiiQtEditorApp::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiMaterialAssetDocument::EditorEventHandler, this));
}

xiiMaterialAssetDocument::~xiiMaterialAssetDocument()
{
  xiiQtEditorApp::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiMaterialAssetDocument::EditorEventHandler, this));
}

void xiiMaterialAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  {
    xiiCommandHistory* pHistory = GetCommandHistory();
    pHistory->StartTransaction("Update Material Shader");
    GetProperties()->SetDocument(this);
    pHistory->FinishTransaction();
  }

  bool bSetModified = false;

  // The above command may patch the doc with the newest shader properties so we need to clear the undo history here.
  GetCommandHistory()->ClearUndoHistory();
  SetModified(bSetModified);
}

xiiDocumentObject* xiiMaterialAssetDocument::GetShaderPropertyObject()
{
  xiiDocumentObject*         pObject        = GetObjectManager()->GetRootObject()->GetChildren()[0];
  xiiIReflectedTypeAccessor& accessor       = pObject->GetTypeAccessor();
  xiiUuid                    propObjectGuid = accessor.GetValue("ShaderProperties").ConvertTo<xiiUuid>();
  xiiDocumentObject*         pPropObject    = nullptr;
  if (propObjectGuid.IsValid())
  {
    pPropObject = GetObjectManager()->GetObject(propObjectGuid);
  }
  return pPropObject;
}

const xiiDocumentObject* xiiMaterialAssetDocument::GetShaderPropertyObject() const
{
  return const_cast<xiiMaterialAssetDocument*>(this)->GetShaderPropertyObject();
}

void xiiMaterialAssetDocument::SetBaseMaterial(const char* szBaseMaterial)
{
  xiiDocumentObject* pObject    = GetPropertyObject();
  auto               pAssetInfo = xiiAssetCurator::GetSingleton()->FindSubAsset(szBaseMaterial);
  if (pAssetInfo == nullptr)
  {
    xiiHybridArray<const xiiDocumentObject*, 2> sel;
    sel.PushBack(pObject);
    UnlinkPrefabs(sel);
  }
  else
  {
    const xiiStringBuilder&       sNewBase   = xiiPrefabCache::GetSingleton()->GetCachedPrefabDocument(pAssetInfo->m_Data.m_Guid);
    const xiiAbstractObjectGraph* pBaseGraph = xiiPrefabCache::GetSingleton()->GetCachedPrefabGraph(pAssetInfo->m_Data.m_Guid);

    xiiUuid seed = GetSeedFromBaseMaterial(pBaseGraph);
    if (sNewBase.IsEmpty() || !pBaseGraph || !seed.IsValid())
    {
      xiiLog::Error("The selected base material '{0}' is not a valid material file!", szBaseMaterial);
      return;
    }

    {
      auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObject->GetGuid());

      if (pMeta->m_CreateFromPrefab != pAssetInfo->m_Data.m_Guid)
      {
        pMeta->m_sBasePrefab      = sNewBase;
        pMeta->m_CreateFromPrefab = pAssetInfo->m_Data.m_Guid;
        pMeta->m_PrefabSeedGuid   = seed;
      }
      m_DocumentObjectMetaData->EndModifyMetaData(xiiDocumentObjectMetaData::PrefabFlag);
    }
    UpdatePrefabs();
  }
}

xiiUuid xiiMaterialAssetDocument::GetSeedFromBaseMaterial(const xiiAbstractObjectGraph* pBaseGraph)
{
  if (!pBaseGraph)
    return xiiUuid();

  xiiUuid instanceGuid = GetPropertyObject()->GetGuid();
  xiiUuid baseGuid     = xiiMaterialAssetDocument::GetMaterialNodeGuid(*pBaseGraph);
  if (baseGuid.IsValid())
  {
    // Create seed that converts base guid into instance guid
    instanceGuid.RevertCombinationWithSeed(baseGuid);
    return instanceGuid;
  }

  return xiiUuid();
}

xiiUuid xiiMaterialAssetDocument::GetMaterialNodeGuid(const xiiAbstractObjectGraph& graph)
{
  for (auto it = graph.GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetType() == xiiGetStaticRTTI<xiiMaterialAssetProperties>()->GetTypeName())
    {
      return it.Value()->GetGuid();
    }
  }
  return xiiUuid();
}

void xiiMaterialAssetDocument::UpdatePrefabObject(xiiDocumentObject* pObject, const xiiUuid& PrefabAsset, const xiiUuid& PrefabSeed, xiiStringView sBasePrefab)
{
  // Base
  xiiAbstractObjectGraph baseGraph;
  xiiPrefabUtils::LoadGraph(baseGraph, sBasePrefab);
  baseGraph.PruneGraph(GetMaterialNodeGuid(baseGraph));

  // NewBase
  const xiiStringBuilder&       sLeft      = xiiPrefabCache::GetSingleton()->GetCachedPrefabDocument(PrefabAsset);
  const xiiAbstractObjectGraph* pLeftGraph = xiiPrefabCache::GetSingleton()->GetCachedPrefabGraph(PrefabAsset);
  xiiAbstractObjectGraph        leftGraph;
  if (pLeftGraph)
  {
    pLeftGraph->Clone(leftGraph);
  }
  else
  {
    xiiStringBuilder sGuid;
    xiiConversionUtils::ToString(PrefabAsset, sGuid);
    xiiLog::Error("Can't update prefab, new base graph does not exist: {0}", sGuid);
    return;
  }
  leftGraph.PruneGraph(GetMaterialNodeGuid(leftGraph));

  // Instance
  xiiAbstractObjectGraph rightGraph;
  {
    xiiDocumentObjectConverterWriter writer(&rightGraph, pObject->GetDocumentObjectManager());
    writer.AddObjectToGraph(pObject);
    rightGraph.ReMapNodeGuids(PrefabSeed, true);
  }

  // Merge diffs relative to base
  xiiDeque<xiiAbstractGraphDiffOperation> mergedDiff;
  xiiPrefabUtils::Merge(baseGraph, leftGraph, rightGraph, mergedDiff);

  // Skip 'ShaderMode' as it should not be inherited, and 'ShaderProperties' is being set by the 'Shader' property
  xiiDeque<xiiAbstractGraphDiffOperation> cleanedDiff;
  for (const xiiAbstractGraphDiffOperation& op : mergedDiff)
  {
    if (op.m_Operation == xiiAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      if (op.m_sProperty == "ShaderMode" || op.m_sProperty == "ShaderProperties")
        continue;

      cleanedDiff.PushBack(op);
    }
  }

  // Apply diff to base, making it the new instance
  baseGraph.ApplyDiff(cleanedDiff);

  // Do not allow 'Shader' to be overridden, always use the prefab template version.
  if (xiiAbstractObjectNode* pNode = leftGraph.GetNode(GetMaterialNodeGuid(leftGraph)))
  {
    if (auto pProp = pNode->FindProperty("Shader"))
    {
      if (xiiAbstractObjectNode* pNodeBase = baseGraph.GetNode(GetMaterialNodeGuid(baseGraph)))
      {
        pNodeBase->ChangeProperty("Shader", pProp->m_Value);
      }
    }
  }

  // Create a new diff that changes our current instance to the new instance
  xiiDeque<xiiAbstractGraphDiffOperation> newInstanceToCurrentInstance;
  baseGraph.CreateDiffWithBaseGraph(rightGraph, newInstanceToCurrentInstance);
  if (false)
  {
    xiiFileWriter file;
    file.Open("C:\\temp\\Material - diff.txt").IgnoreResult();

    xiiStringBuilder sDiff;
    sDiff.Append("######## New Instance To Instance #######\n");
    xiiPrefabUtils::WriteDiff(newInstanceToCurrentInstance, sDiff);
    file.WriteBytes(sDiff.GetData(), sDiff.GetElementCount()).IgnoreResult();
  }
  // Apply diff to current instance
  // Shader needs to be set first
  for (xiiUInt32 i = 0; i < newInstanceToCurrentInstance.GetCount(); ++i)
  {
    if (newInstanceToCurrentInstance[i].m_sProperty == "Shader")
    {
      xiiAbstractGraphDiffOperation op = newInstanceToCurrentInstance[i];
      newInstanceToCurrentInstance.RemoveAtAndCopy(i);
      newInstanceToCurrentInstance.InsertAt(0, op);
      break;
    }
  }
  for (const xiiAbstractGraphDiffOperation& op : newInstanceToCurrentInstance)
  {
    if (op.m_Operation == xiiAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      // Never change this material's mode, as it should not be inherited from prefab base
      if (op.m_sProperty == "ShaderMode")
        continue;

      // these properties may not exist and we do not want to change them either
      if (op.m_sProperty == "MetaBasePrefab" || op.m_sProperty == "MetaPrefabSeed" || op.m_sProperty == "MetaFromPrefab")
        continue;

      xiiSetObjectPropertyCommand cmd;
      cmd.m_Object = op.m_Node;
      cmd.m_Object.CombineWithSeed(PrefabSeed);
      cmd.m_NewValue  = op.m_Value;
      cmd.m_sProperty = op.m_sProperty;

      auto pObj = GetObjectAccessor()->GetObject(cmd.m_Object);
      if (!pObj)
        continue;

      auto pProp = pObj->GetType()->FindPropertyByName(op.m_sProperty);
      if (!pProp)
        continue;

      if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        continue;

      GetCommandHistory()->AddCommand(cmd).AssertSuccess();
    }
  }

  // Update prefab meta data
  {
    auto pMeta                = m_DocumentObjectMetaData->BeginModifyMetaData(pObject->GetGuid());
    pMeta->m_CreateFromPrefab = PrefabAsset; // Should not change
    pMeta->m_PrefabSeedGuid   = PrefabSeed;  // Should not change
    pMeta->m_sBasePrefab      = sLeft;

    m_DocumentObjectMetaData->EndModifyMetaData(xiiDocumentObjectMetaData::PrefabFlag);
  }
}

class xiiVisualShaderErrorLog : public xiiLogInterface
{
public:
  xiiStringBuilder m_sResult;
  xiiResult        m_Status;

  xiiVisualShaderErrorLog() :
    m_Status(XII_SUCCESS)
  {
  }

  virtual void HandleLogMessage(const xiiLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case xiiLogMsgType::ErrorMsg:
        m_Status = XII_FAILURE;
        m_sResult.Append("Error: ", le.m_sText, "\n");
        break;

      case xiiLogMsgType::SeriousWarningMsg:
      case xiiLogMsgType::WarningMsg:
        m_sResult.Append("Warning: ", le.m_sText, "\n");
        break;

      default:
        return;
    }
  }
};

xiiTransformStatus xiiMaterialAssetDocument::InternalTransformAsset(const char* szTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  if (sOutputTag.IsEqual(xiiMaterialAssetDocumentManager::s_szShaderOutputTag))
  {
    xiiStatus ret = RecreateVisualShaderFile(AssetHeader);

    if (transformFlags.IsSet(xiiTransformFlags::ForceTransform))
    {
      xiiMaterialVisualShaderEvent e;

      if (GetProperties()->m_ShaderMode == xiiMaterialShaderMode::Custom)
      {
        e.m_Type            = xiiMaterialVisualShaderEvent::TransformFailed;
        e.m_sTransformError = ret.m_sMessage;

        if (ret.Succeeded())
        {
          e.m_Type                        = xiiMaterialVisualShaderEvent::TransformSucceeded;
          xiiStringBuilder sAutoGenShader = GetProperties()->GetAutoGenShaderPathAbs();

          QStringList      arguments;
          xiiStringBuilder temp;

          arguments << "-project";
          arguments << QString::fromUtf8(xiiToolsProject::GetSingleton()->GetProjectDirectory().GetData());

          arguments << "-shader";
          arguments << QString::fromUtf8(sAutoGenShader.GetData());

          arguments << "-platform";
          arguments << "DX11_SM50"; /// \todo Rendering platform is currently hardcoded

          // determine the permutation variables that should get fixed values
          {
            // m_sCheckPermutations are just all fixed perm vars from every node in the VS
            xiiStringBuilder        temp = m_sCheckPermutations;
            xiiDeque<xiiStringView> perms;
            temp.Split(false, perms, "\n");

            // remove duplicates
            xiiSet<xiiString> uniquePerms;
            for (const xiiStringView& perm : perms)
            {
              uniquePerms.Insert(perm);
            }

            // pass permutation variable definitions to the compiler: "SOME_VAR=SOME_VAL"
            arguments << "-perm";
            for (auto it = uniquePerms.GetIterator(); it.IsValid(); ++it)
            {
              arguments << it.Key().GetData();
            }
          }

          xiiVisualShaderErrorLog log;

          ret = xiiQtEditorApp::GetSingleton()->ExecuteTool("xiiShaderCompiler", arguments, 60, &log);
          if (ret.Failed())
          {
            e.m_Type            = xiiMaterialVisualShaderEvent::TransformFailed;
            e.m_sTransformError = ret.m_sMessage;
          }
          else
          {
            e.m_Type            = log.m_Status.Succeeded() ? xiiMaterialVisualShaderEvent::TransformSucceeded : xiiMaterialVisualShaderEvent::TransformFailed;
            e.m_sTransformError = log.m_sResult;
            xiiLog::Info("Compiled Visual Shader.");
          }
        }
      }
      else
      {
        e.m_Type = xiiMaterialVisualShaderEvent::VisualShaderNotUsed;
      }

      if (e.m_Type == xiiMaterialVisualShaderEvent::TransformFailed)
      {
        TagVisualShaderFileInvalid(pAssetProfile, e.m_sTransformError);
      }

      m_VisualShaderEvents.Broadcast(e);
    }

    return ret;
  }
  else
  {
    return SUPER::InternalTransformAsset(szTargetFile, sOutputTag, pAssetProfile, AssetHeader, transformFlags);
  }
}

xiiTransformStatus xiiMaterialAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  XII_ASSERT_DEV(sOutputTag.IsEmpty(), "Additional output '{0}' not implemented!", sOutputTag);

  return WriteMaterialAsset(stream, pAssetProfile, true);
}

xiiTransformStatus xiiMaterialAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  return xiiAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
}

void xiiMaterialAssetDocument::InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const
{
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void xiiMaterialAssetDocument::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void xiiMaterialAssetDocument::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void xiiMaterialAssetDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (!GetProperties()->m_sAssetFilterTags.IsEmpty())
  {
    const xiiStringBuilder tags(";", GetProperties()->m_sAssetFilterTags, ";");

    pInfo->m_sAssetsDocumentTags = tags;
  }

  if (GetProperties()->m_ShaderMode != xiiMaterialShaderMode::BaseMaterial)
  {
    // remove base material dependency, if it isn't used
    pInfo->m_TransformDependencies.Remove(GetProperties()->GetBaseMaterial());
    pInfo->m_ThumbnailDependencies.Remove(GetProperties()->GetBaseMaterial());
  }

  if (GetProperties()->m_ShaderMode != xiiMaterialShaderMode::File)
  {
    const bool bInUseByBaseMaterial = GetProperties()->m_ShaderMode == xiiMaterialShaderMode::BaseMaterial && xiiStringUtils::IsEqual(GetProperties()->GetShader(), GetProperties()->GetBaseMaterial());

    // remove shader file dependency, if it isn't used and differs from the base material
    if (!bInUseByBaseMaterial)
    {
      pInfo->m_TransformDependencies.Remove(GetProperties()->GetShader());
      pInfo->m_ThumbnailDependencies.Remove(GetProperties()->GetShader());
    }
  }

  if (GetProperties()->m_ShaderMode == xiiMaterialShaderMode::Custom)
  {
    // We write our own guid into the shader field so BaseMaterial materials can find the shader file.
    // This would cause us to have a dependency to ourselves so we need to remove it.
    xiiStringBuilder tmp;
    pInfo->m_TransformDependencies.Remove(xiiConversionUtils::ToString(GetGuid(), tmp));
    pInfo->m_ThumbnailDependencies.Remove(xiiConversionUtils::ToString(GetGuid(), tmp));

    xiiVisualShaderCodeGenerator codeGen;

    xiiSet<xiiString> cfgFiles;
    codeGen.DetermineConfigFileDependencies(static_cast<const xiiDocumentNodeManager*>(GetObjectManager()), cfgFiles);

    for (const auto& sCfgFile : cfgFiles)
    {
      pInfo->m_TransformDependencies.Insert(sCfgFile);
    }

    pInfo->m_Outputs.Insert(xiiMaterialAssetDocumentManager::s_szShaderOutputTag);

    /// \todo The Visual Shader node configuration files would need to be a dependency of the auto-generated shader.
  }
}

xiiStatus xiiMaterialAssetDocument::WriteMaterialAsset(xiiStreamWriter& inout_stream0, const xiiPlatformProfile* pAssetProfile, bool bEmbedLowResData) const
{
  const xiiMaterialAssetProperties* pProp = GetProperties();

  xiiStringBuilder sValue;

  // now generate the .xiiBinMaterial file
  {
    const xiiUInt8 uiVersion = 7;

    inout_stream0 << uiVersion;

    xiiUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    uiCompressionMode = 1;
    xiiCompressedStreamWriterZstd stream(&inout_stream0, 0, xiiCompressedStreamWriterZstd::Compression::Average);
#else
    xiiStreamWriter& stream = stream0;
#endif

    inout_stream0 << uiCompressionMode;

    stream << pProp->m_sBaseMaterial;
    stream << pProp->m_sSurface;

    xiiString sRelativeShaderPath = pProp->ResolveRelativeShaderPath();
    stream << sRelativeShaderPath;

    xiiHybridArray<const xiiAbstractProperty*, 16> Textures2D;
    xiiHybridArray<const xiiAbstractProperty*, 16> TexturesCube;
    xiiHybridArray<const xiiAbstractProperty*, 16> Permutations;
    xiiHybridArray<const xiiAbstractProperty*, 16> Constants;

    const xiiDocumentObject* pObject = GetShaderPropertyObject();
    if (pObject != nullptr)
    {
      bool                                           hasBaseMaterial = xiiPrefabUtils::GetPrefabRoot(pObject, *m_DocumentObjectMetaData).IsValid();
      auto                                           pType           = pObject->GetTypeAccessor().GetType();
      xiiHybridArray<const xiiAbstractProperty*, 32> properties;
      pType->GetAllProperties(properties);

      xiiHybridArray<xiiPropertySelection, 1> selection;
      selection.PushBack({pObject, xiiVariant()});
      xiiDefaultObjectState defaultState(GetObjectAccessor(), selection.GetArrayPtr());

      for (auto pProp : properties)
      {
        if (hasBaseMaterial && defaultState.IsDefaultValue(pProp))
          continue;

        const xiiCategoryAttribute* pCategory = pProp->GetAttributeByType<xiiCategoryAttribute>();

        XII_ASSERT_DEBUG(pCategory, "Category cannot be null for a shader property");
        if (pCategory == nullptr)
          continue;

        if (pCategory->GetCategory() == "Texture 2D")
        {
          Textures2D.PushBack(pProp);
        }
        else if (pCategory->GetCategory() == "Texture Cube")
        {
          TexturesCube.PushBack(pProp);
        }
        else if (pCategory->GetCategory() == "Permutation")
        {
          Permutations.PushBack(pProp);
        }
        else if (pCategory->GetCategory() == "Constant")
        {
          Constants.PushBack(pProp);
        }
        else
        {
          XII_REPORT_FAILURE("Invalid shader property type '{0}'", pCategory->GetCategory());
        }
      }
    }

    // write out the permutation variables
    {
      const xiiUInt16 uiPermVars = Permutations.GetCount();
      stream << uiPermVars;

      for (auto pProp : Permutations)
      {
        XII_ASSERT_DEBUG(pObject != nullptr, "Need object to write out permutation");
        xiiStringView sName = pProp->GetPropertyName();
        if (pProp->GetSpecificType()->GetVariantType() == xiiVariantType::Bool)
        {
          sValue = pObject->GetTypeAccessor().GetValue(sName).Get<bool>() ? "TRUE" : "FALSE";
        }
        else if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
        {
          xiiReflectionUtils::EnumerationToString(pProp->GetSpecificType(), pObject->GetTypeAccessor().GetValue(sName).ConvertTo<xiiInt64>(), sValue, xiiReflectionUtils::EnumConversionMode::ValueNameOnly);
        }
        else
        {
          XII_REPORT_FAILURE("Invalid shader permutation property type '{0}'", pProp->GetSpecificType()->GetTypeName());
        }

        stream << sName;
        stream << sValue;
      }
    }

    // write out the 2D textures
    {
      const xiiUInt16 uiTextures = Textures2D.GetCount();
      stream << uiTextures;

      for (auto pProp : Textures2D)
      {
        XII_ASSERT_DEBUG(pObject != nullptr, "Need object to write out texture");
        xiiStringView sName = pProp->GetPropertyName();
        sValue              = pObject->GetTypeAccessor().GetValue(sName).ConvertTo<xiiString>();

        stream << sName;
        stream << sValue;
      }
    }

    // write out the Cube textures
    {
      const xiiUInt16 uiTextures = TexturesCube.GetCount();
      stream << uiTextures;

      for (auto pProp : TexturesCube)
      {
        XII_ASSERT_DEBUG(pObject != nullptr, "Need object to write out texture cube");
        xiiStringView sName = pProp->GetPropertyName();
        sValue              = pObject->GetTypeAccessor().GetValue(sName).ConvertTo<xiiString>();

        stream << sName;
        stream << sValue;
      }
    }

    // write out the constants
    {
      const xiiUInt16 uiConstants = Constants.GetCount();
      stream << uiConstants;

      for (auto pProp : Constants)
      {
        XII_ASSERT_DEBUG(pObject != nullptr, "Need object to write out constant");
        xiiStringView sName = pProp->GetPropertyName();
        xiiVariant    value = pObject->GetTypeAccessor().GetValue(sName);

        stream << sName;
        stream << value;
      }
    }

    // render data category
    {
      xiiVariantDictionary materialConfig;
      if (pObject != nullptr)
      {
        XII_SUCCEED_OR_RETURN(ParseMaterialConfig(sRelativeShaderPath, pObject, materialConfig));
      }

      xiiVariant renderDataCategory;
      materialConfig.TryGetValue("RenderDataCategory", renderDataCategory);

      stream << renderDataCategory.ConvertTo<xiiString>();
    }

    // find and embed low res texture data
    {
      if (bEmbedLowResData)
      {
        xiiStringBuilder           sFilename, sResourceName;
        xiiDynamicArray<xiiUInt32> content;

        // embed 2D texture data
        for (auto prop : Textures2D)
        {
          XII_ASSERT_DEBUG(pObject != nullptr, "Need object to write out texture2d");
          xiiStringView sName = prop->GetPropertyName();
          sValue              = pObject->GetTypeAccessor().GetValue(sName).ConvertTo<xiiString>();

          if (sValue.IsEmpty())
            continue;

          sResourceName = sValue;

          auto asset = xiiAssetCurator::GetSingleton()->FindSubAsset(sValue);
          if (!asset.isValid())
            continue;

          sValue = asset->m_pAssetInfo->GetManager()->GetAbsoluteOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, asset->m_pAssetInfo->m_Path, "", pAssetProfile);

          sFilename = sValue.GetFileName();
          sFilename.Append("-lowres");

          sValue.ChangeFileName(sFilename);

          xiiFileReader file;
          if (file.Open(sValue).Failed())
            continue;

          content.SetCountUninitialized(file.GetFileSize());

          file.ReadBytes(content.GetData(), content.GetCount());

          stream << sResourceName;
          stream << content.GetCount();
          XII_SUCCEED_OR_RETURN(stream.WriteBytes(content.GetData(), content.GetCount()));
        }
      }

      // marker: end of embedded data
      stream << "";
    }

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    XII_SUCCEED_OR_RETURN(stream.FinishCompressedStream());

    xiiLog::Dev("Compressed material data from {0} KB to {1} KB ({2}%%)", xiiArgF((float)stream.GetUncompressedSize() / 1024.0f, 1), xiiArgF((float)stream.GetCompressedSize() / 1024.0f, 1), xiiArgF(100.0f * stream.GetCompressedSize() / stream.GetUncompressedSize(), 1));
#endif
  }

  return xiiStatus(XII_SUCCESS);
}

void xiiMaterialAssetDocument::TagVisualShaderFileInvalid(const xiiPlatformProfile* pAssetProfile, const char* szError)
{
  if (GetProperties()->m_ShaderMode != xiiMaterialShaderMode::Custom)
    return;

  xiiAssetDocumentManager* pManager       = xiiDynamicCast<xiiAssetDocumentManager*>(GetDocumentManager());
  xiiString                sAutoGenShader = pManager->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), xiiMaterialAssetDocumentManager::s_szShaderOutputTag);

  xiiStringBuilder all;

  // read shader source
  {
    xiiFileReader file;
    if (file.Open(sAutoGenShader).Failed())
      return;

    all.ReadAll(file);
  }

  all.PrependFormat("/*\n{0}\n*/\n", szError);

  // write adjusted shader source
  {
    xiiFileWriter fileOut;
    if (fileOut.Open(sAutoGenShader).Failed())
      return;

    fileOut.WriteBytes(all.GetData(), all.GetElementCount()).IgnoreResult();
  }
}

xiiStatus xiiMaterialAssetDocument::RecreateVisualShaderFile(const xiiAssetFileHeader& assetHeader)
{
  if (GetProperties()->m_ShaderMode != xiiMaterialShaderMode::Custom)
  {
    return xiiStatus(XII_SUCCESS);
  }

  xiiAssetDocumentManager* pManager       = xiiDynamicCast<xiiAssetDocumentManager*>(GetDocumentManager());
  xiiString                sAutoGenShader = pManager->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), xiiMaterialAssetDocumentManager::s_szShaderOutputTag);

  xiiVisualShaderCodeGenerator codeGen;

  XII_SUCCEED_OR_RETURN(codeGen.GenerateVisualShader(static_cast<const xiiDocumentNodeManager*>(GetObjectManager()), m_sCheckPermutations));

  xiiFileWriter file;
  if (file.Open(sAutoGenShader).Succeeded())
  {
    xiiStringBuilder shader = codeGen.GetFinalShaderCode();
    shader.PrependFormat("//{0}|{1}\n", assetHeader.GetFileHash(), assetHeader.GetFileVersion());

    XII_SUCCEED_OR_RETURN(file.WriteBytes(shader.GetData(), shader.GetElementCount()));
    file.Close();

    InvalidateCachedShader();

    return xiiStatus(XII_SUCCESS);
  }
  else
    return xiiStatus(xiiFmt("Failed to write auto-generated shader to '{0}'", sAutoGenShader));
}

void xiiMaterialAssetDocument::InvalidateCachedShader()
{
  xiiAssetDocumentManager* pManager = xiiDynamicCast<xiiAssetDocumentManager*>(GetDocumentManager());
  xiiString                sShader;

  if (GetProperties()->m_ShaderMode == xiiMaterialShaderMode::Custom)
  {
    sShader = pManager->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), xiiMaterialAssetDocumentManager::s_szShaderOutputTag);
  }
  else
  {
    sShader = GetProperties()->GetShader();
  }

  // This should update the shader parameter section in all affected materials
  xiiShaderTypeRegistry::GetSingleton()->GetShaderType(sShader);
}

void xiiMaterialAssetDocument::EditorEventHandler(const xiiEditorAppEvent& e)
{
  if (e.m_Type == xiiEditorAppEvent::Type::ReloadResources)
  {
    InvalidateCachedShader();
  }
}

static void MarkReachableNodes(xiiMap<const xiiDocumentObject*, bool>& ref_allNodes, const xiiDocumentObject* pRoot, xiiDocumentNodeManager* pNodeManager)
{
  if (ref_allNodes[pRoot])
    return;

  ref_allNodes[pRoot] = true;

  auto allInputs = pNodeManager->GetInputPins(pRoot);

  // we start at the final output, so use the inputs on a node and then walk backwards
  for (auto& pTargetPin : allInputs)
  {
    auto connections = pNodeManager->GetConnections(*pTargetPin);

    // all incoming connections at the input pin, there should only be one though
    for (const xiiConnection* const pConnection : connections)
    {
      // output pin on other node connecting to this node
      const xiiPin& sourcePin = pConnection->GetSourcePin();

      // recurse from here
      MarkReachableNodes(ref_allNodes, sourcePin.GetParent(), pNodeManager);
    }
  }
}

void xiiMaterialAssetDocument::RemoveDisconnectedNodes()
{
  xiiDocumentNodeManager* pNodeManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());

  const xiiDocumentObject* pRoot         = pNodeManager->GetRootObject();
  const xiiRTTI*           pNodeBaseRtti = xiiVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

  const xiiHybridArray<xiiDocumentObject*, 8>& children = pRoot->GetChildren();
  xiiMap<const xiiDocumentObject*, bool>       AllNodes;

  for (xiiUInt32 i = 0; i < children.GetCount(); ++i)
  {
    if (children[i]->GetType()->IsDerivedFrom(pNodeBaseRtti))
    {
      AllNodes[children[i]] = false;
    }
  }

  for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
  {
    // skip nodes that have already been marked
    if (it.Value())
      continue;

    auto pDesc = xiiVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(it.Key()->GetType());

    if (pDesc->m_NodeType == xiiVisualShaderNodeType::Main)
    {
      MarkReachableNodes(AllNodes, it.Key(), pNodeManager);
    }
  }

  // now purge all nodes that haven't been reached
  {
    auto pHistory = GetCommandHistory();
    pHistory->StartTransaction("Purge unreachable nodes");

    for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
    {
      // skip nodes that have been marked
      if (it.Value())
        continue;

      xiiRemoveNodeCommand rem;
      rem.m_Object = it.Key()->GetGuid();

      pHistory->AddCommand(rem).AssertSuccess();
    }

    pHistory->FinishTransaction();
  }
}

xiiUuid xiiMaterialAssetDocument::GetLitBaseMaterial()
{
  if (!s_LitBaseMaterial.IsValid())
  {
    static const char* szLitMaterialAssetPath = xiiMaterialResource::GetDefaultMaterialFileName(xiiMaterialResource::DefaultMaterialType::Lit);
    auto               assetInfo              = xiiAssetCurator::GetSingleton()->FindSubAsset(szLitMaterialAssetPath);
    if (assetInfo)
      s_LitBaseMaterial = assetInfo->m_Data.m_Guid;
    else
      xiiLog::Error("Can't find default lit material {0}", szLitMaterialAssetPath);
  }
  return s_LitBaseMaterial;
}

xiiUuid xiiMaterialAssetDocument::GetLitAlphaTestBaseMaterial()
{
  if (!s_LitAlphaTextBaseMaterial.IsValid())
  {
    static const char* szLitAlphaTestMaterialAssetPath = xiiMaterialResource::GetDefaultMaterialFileName(xiiMaterialResource::DefaultMaterialType::LitAlphaTest);
    auto               assetInfo                       = xiiAssetCurator::GetSingleton()->FindSubAsset(szLitAlphaTestMaterialAssetPath);
    if (assetInfo)
      s_LitAlphaTextBaseMaterial = assetInfo->m_Data.m_Guid;
    else
      xiiLog::Error("Can't find default lit alpha test material {0}", szLitAlphaTestMaterialAssetPath);
  }
  return s_LitAlphaTextBaseMaterial;
}

xiiUuid xiiMaterialAssetDocument::GetNeutralNormalMap()
{
  if (!s_NeutralNormalMap.IsValid())
  {
    static const char* szNeutralNormalMapAssetPath = "Base/Textures/NeutralNormal.xiiTextureAsset";
    auto               assetInfo                   = xiiAssetCurator::GetSingleton()->FindSubAsset(szNeutralNormalMapAssetPath);
    if (assetInfo)
      s_NeutralNormalMap = assetInfo->m_Data.m_Guid;
    else
      xiiLog::Error("Can't find neutral normal map texture {0}", szNeutralNormalMapAssetPath);
  }
  return s_NeutralNormalMap;
}

void xiiMaterialAssetDocument::GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/xiiEditor.NodeGraph");
}

bool xiiMaterialAssetDocument::CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_sMimeType) const
{
  out_sMimeType = "application/xiiEditor.NodeGraph";

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool xiiMaterialAssetDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, xiiQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiMaterialAssetPropertiesPatch_1_2 : public xiiGraphPatch
{
public:
  xiiMaterialAssetPropertiesPatch_1_2() :
    xiiGraphPatch("xiiMaterialAssetProperties", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Shader Mode", "ShaderMode");
    pNode->RenameProperty("Base Material", "BaseMaterial");
  }
};

xiiMaterialAssetPropertiesPatch_1_2 g_xiiMaterialAssetPropertiesPatch_1_2;


class xiiMaterialAssetPropertiesPatch_2_3 : public xiiGraphPatch
{
public:
  xiiMaterialAssetPropertiesPatch_2_3() :
    xiiGraphPatch("xiiMaterialAssetProperties", 3)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    auto* pBaseMatProp    = pNode->FindProperty("BaseMaterial");
    auto* pShaderModeProp = pNode->FindProperty("ShaderMode");
    if (pBaseMatProp && pBaseMatProp->m_Value.IsA<xiiString>())
    {
      if (!pBaseMatProp->m_Value.Get<xiiString>().IsEmpty())
      {
        // BaseMaterial is set
        pNode->ChangeProperty("ShaderMode", (xiiInt32)xiiMaterialShaderMode::BaseMaterial);
      }
      else
      {
        pNode->ChangeProperty("ShaderMode", (xiiInt32)xiiMaterialShaderMode::File);
      }
    }
  }
};

xiiMaterialAssetPropertiesPatch_2_3 g_xiiMaterialAssetPropertiesPatch_2_3;

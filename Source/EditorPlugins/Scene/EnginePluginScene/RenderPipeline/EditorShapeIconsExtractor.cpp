#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/RenderPipeline/EditorShapeIconsExtractor.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Components/SpriteComponent.h>
#include <GraphicsCore/Pipeline/View.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorShapeIconsExtractor, 1, xiiRTTIDefaultAllocator<xiiEditorShapeIconsExtractor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1.0f), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("MaxScreenSize", m_fMaxScreenSize)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(64.0f), new xiiSuffixAttribute(" px")),
    XII_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiEditorShapeIconsExtractor::xiiEditorShapeIconsExtractor(const char* szName) :
  xiiExtractor(szName)
{
  m_fSize          = 1.0f;
  m_fMaxScreenSize = 64.0f;
  m_pSceneContext  = nullptr;

  FillShapeIconInfo();
}

xiiEditorShapeIconsExtractor::~xiiEditorShapeIconsExtractor() = default;

void xiiEditorShapeIconsExtractor::Extract(
  const xiiView&                               view,
  const xiiDynamicArray<const xiiGameObject*>& visibleObjects,
  xiiExtractedRenderData&                      ref_extractedRenderData)
{
  XII_LOCK(view.GetWorld()->GetReadMarker());

  /// \todo Once we have a solution for objects that only have a shape icon we can switch this loop to use visibleObjects instead.
  for (auto it = view.GetWorld()->GetObjects(); it.IsValid(); ++it)
  {
    const xiiGameObject* pObject = it;
    if (FilterByViewTags(view, pObject))
      continue;

    ExtractShapeIcon(pObject, view, ref_extractedRenderData, xiiDefaultRenderDataCategories::SimpleOpaque);
  }

  if (m_pSceneContext != nullptr)
  {
    auto objects = m_pSceneContext->GetSelectionWithChildren();

    for (const auto& hObject : objects)
    {
      const xiiGameObject* pObject = nullptr;
      if (view.GetWorld()->TryGetObject(hObject, pObject))
      {
        if (FilterByViewTags(view, pObject))
          continue;

        ExtractShapeIcon(pObject, view, ref_extractedRenderData, xiiDefaultRenderDataCategories::Selection);
      }
    }
  }
}

xiiResult xiiEditorShapeIconsExtractor::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fSize;
  inout_stream << m_fMaxScreenSize;
  return XII_SUCCESS;
}

xiiResult xiiEditorShapeIconsExtractor::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fSize;
  inout_stream >> m_fMaxScreenSize;
  return XII_SUCCESS;
}

void xiiEditorShapeIconsExtractor::ExtractShapeIcon(const xiiGameObject* pObject, const xiiView& view, xiiExtractedRenderData& extractedRenderData, xiiRenderData::Category category)
{
  static const xiiTag& tagHidden = xiiTagRegistry::GetGlobalRegistry().RegisterTag("EditorHidden");
  static const xiiTag& tagEditor = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

  if (pObject->GetTags().IsSet(tagEditor) || pObject->GetTags().IsSet(tagHidden))
    return;

  if (pObject->WasCreatedByPrefab())
    return;

  if (pObject->GetComponents().IsEmpty())
    return;

  const xiiComponent* pComponent = nullptr;
  for (auto it : pObject->GetComponents())
  {
    if (it->IsActive())
    {
      pComponent = it;
      break;
    }
  }

  if (pComponent == nullptr)
    return;

  const xiiRTTI* pRtti = pComponent->GetDynamicRTTI();

  ShapeIconInfo* pShapeIconInfo = nullptr;
  if (m_ShapeIconInfos.TryGetValue(pRtti, pShapeIconInfo))
  {
    xiiSpriteRenderData* pRenderData = xiiCreateRenderDataForThisFrame<xiiSpriteRenderData>(pObject);
    {
      pRenderData->m_GlobalTransform = pObject->GetGlobalTransform();
      pRenderData->m_GlobalBounds    = pObject->GetGlobalBounds();
      pRenderData->m_hTexture        = pShapeIconInfo->m_hTexture;
      pRenderData->m_fSize           = m_fSize;
      pRenderData->m_fMaxScreenSize  = m_fMaxScreenSize;
      pRenderData->m_fAspectRatio    = 1.0f;
      pRenderData->m_BlendMode       = xiiSpriteBlendMode::ShapeIcon;
      pRenderData->m_texCoordScale   = xiiVec2(1.0f);
      pRenderData->m_texCoordOffset  = xiiVec2(0.0f);
      pRenderData->m_uiUniqueID      = xiiRenderComponent::GetUniqueIdForRendering(pComponent);

      // prefer color gamma properties
      if (pShapeIconInfo->m_pColorGammaProperty != nullptr)
      {
        pRenderData->m_color = xiiColor(pShapeIconInfo->m_pColorGammaProperty->GetValue(pComponent));
      }
      else if (pShapeIconInfo->m_pColorProperty != nullptr)
      {
        pRenderData->m_color = pShapeIconInfo->m_pColorProperty->GetValue(pComponent);
      }
      else
      {
        pRenderData->m_color = pShapeIconInfo->m_FallbackColor;
      }

      pRenderData->m_color.a = 1.0f;

      pRenderData->FillBatchIdAndSortingKey();
    }

    extractedRenderData.AddRenderData(pRenderData, category);
  }
}

const xiiTypedMemberProperty<xiiColor>* xiiEditorShapeIconsExtractor::FindColorProperty(const xiiRTTI* pRtti) const
{
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (const xiiAbstractProperty* pProperty : properties)
  {
    if (pProperty->GetCategory() == xiiPropertyCategory::Member && pProperty->GetSpecificType() == xiiGetStaticRTTI<xiiColor>())
    {
      return static_cast<const xiiTypedMemberProperty<xiiColor>*>(pProperty);
    }
  }

  return nullptr;
}

const xiiTypedMemberProperty<xiiColorGammaUB>* xiiEditorShapeIconsExtractor::FindColorGammaProperty(const xiiRTTI* pRtti) const
{
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (const xiiAbstractProperty* pProperty : properties)
  {
    if (pProperty->GetCategory() == xiiPropertyCategory::Member && pProperty->GetSpecificType() == xiiGetStaticRTTI<xiiColorGammaUB>())
    {
      return static_cast<const xiiTypedMemberProperty<xiiColorGammaUB>*>(pProperty);
    }
  }

  return nullptr;
}

void xiiEditorShapeIconsExtractor::FillShapeIconInfo()
{
  XII_LOG_BLOCK("LoadShapeIconTextures");

  xiiStringBuilder sPath;

  xiiRTTI::ForEachDerivedType<xiiComponent>(
    [&](const xiiRTTI* pRtti) {
      sPath.Set("Editor/ShapeIcons/", pRtti->GetTypeName(), ".dds");

      if (xiiFileSystem::ExistsFile(sPath))
      {
        auto& shapeIconInfo                 = m_ShapeIconInfos[pRtti];
        shapeIconInfo.m_hTexture            = xiiResourceManager::LoadResource<xiiTexture2DResource>(sPath);
        shapeIconInfo.m_pColorProperty      = FindColorProperty(pRtti);
        shapeIconInfo.m_pColorGammaProperty = FindColorGammaProperty(pRtti);

        if (auto pCatAttribute = pRtti->GetAttributeByType<xiiCategoryAttribute>())
        {
          shapeIconInfo.m_FallbackColor = xiiColorScheme::GetCategoryColor(pCatAttribute->GetCategory(), xiiColorScheme::CategoryColorUsage::ViewportIcon);
        }
      }
    });
}

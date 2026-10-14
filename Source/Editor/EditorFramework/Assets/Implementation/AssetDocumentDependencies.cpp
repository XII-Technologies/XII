/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/PropertyGrid/PrefabDefaultStateProvider.h>

namespace
{
  template <typename Attributes>
  xiiBitflags<xiiDependencyFlags> GetDependencyFlags(const Attributes& attributes)
  {
    xiiBitflags<xiiDependencyFlags> flags;
    for (const auto* attribute : attributes)
    {
      if (attribute->GetDynamicRTTI()->IsDerivedFrom<xiiTemporaryAttribute>())
        return {};
      if (const auto* asset = xiiDynamicCast<const xiiAssetBrowserAttribute*>(attribute))
        flags |= asset->GetDependencyFlags();
      if (const auto* file = xiiDynamicCast<const xiiFileBrowserAttribute*>(attribute))
        flags |= file->GetDependencyFlags();
    }
    return flags;
  }

  // Only explicitly annotated values reach this function. Ordinary strings and unknown exposed
  // parameter names must never become dependencies merely because they resemble asset paths.
  void AddValueReferences(const xiiVariant& value, xiiBitflags<xiiDependencyFlags> flags, xiiAssetDocumentInfo& info)
  {
    if (flags.IsNoFlagSet())
      return;
    if (value.IsA<xiiString>() || value.IsA<xiiStringView>())
    {
      const auto reference = value.ConvertTo<xiiString>();
      if (reference.IsEmpty())
        return;
      if (flags.IsSet(xiiDependencyFlags::Transform))
        info.m_TransformDependencies.Insert(reference);
      if (flags.IsSet(xiiDependencyFlags::Thumbnail))
        info.m_ThumbnailDependencies.Insert(reference);
      if (flags.IsSet(xiiDependencyFlags::Package))
        info.m_PackageDependencies.Insert(reference);
    }
    else if (value.IsA<xiiVariantArray>())
    {
      for (const auto& element : value.Get<xiiVariantArray>())
        AddValueReferences(element, flags, info);
    }
    else if (value.IsA<xiiVariantDictionary>())
    {
      for (auto it = value.Get<xiiVariantDictionary>().GetIterator(); it.IsValid(); ++it)
        AddValueReferences(it.Value(), flags, info);
    }
  }

  void GetParameterDependencyFlags(const xiiDocumentObject* object, const xiiAbstractProperty* property, xiiMap<xiiString, xiiBitflags<xiiDependencyFlags>>& out_flags)
  {
    const auto* exposed = property->GetAttributeByType<xiiExposedParametersAttribute>();
    auto*       curator = xiiAssetCurator::GetSingleton();
    if (exposed == nullptr || curator == nullptr)
      return;
    const auto* sourceProperty = object->GetType()->FindPropertyByName(exposed->GetParametersSource());
    if (sourceProperty == nullptr)
      return;
    const auto source = object->GetTypeAccessor().GetValue(sourceProperty->GetPropertyName());
    if (!source.IsA<xiiString>() && !source.IsA<xiiStringView>())
      return;
    const auto reference = source.ConvertTo<xiiString>();
    if (reference.IsEmpty())
      return;
    const auto asset = curator->FindSubAsset(reference);
    if (!asset)
      return;
    if (const auto* parameters = asset->m_pAssetInfo->m_Info->GetMetaInfo<xiiExposedParameters>())
    {
      for (const auto* parameter : parameters->m_Parameters)
        out_flags.Insert(parameter->m_sName, GetDependencyFlags(parameter->m_Attributes));
    }
    // Copy only flags: metadata is protected by the curator lock and can change on source reload.
  }
} // namespace

void xiiAssetDocument::AddReferences(const xiiDocumentObject* pObject, xiiAssetDocumentInfo* pInfo, bool bInsidePrefab) const
{
  {
    const auto* metadata = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());
    if (metadata->m_CreateFromPrefab.IsValid())
    {
      bInsidePrefab = true;
      xiiStringBuilder reference;
      xiiConversionUtils::ToString(metadata->m_CreateFromPrefab, reference);
      pInfo->m_TransformDependencies.Insert(reference);
      pInfo->m_ThumbnailDependencies.Insert(reference);
    }
    m_DocumentObjectMetaData->EndReadMetaData();
  }

  const auto*                                    type = pObject->GetType();
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  type->GetAllProperties(properties);
  for (const auto* property : properties)
  {
    if (property->GetAttributeByType<xiiTemporaryAttribute>() != nullptr)
      continue;
    const auto                                         flags = GetDependencyFlags(property->GetAttributes());
    xiiMap<xiiString, xiiBitflags<xiiDependencyFlags>> parameterFlags;
    if (property->GetCategory() == xiiPropertyCategory::Map)
      GetParameterDependencyFlags(pObject, property, parameterFlags);
    if (flags.IsNoFlagSet() && parameterFlags.IsEmpty())
      continue;
    const auto variantType = property->GetSpecificType()->GetVariantType();
    if (variantType != xiiVariantType::String && variantType != xiiVariantType::StringView && property->GetSpecificType() != xiiGetStaticRTTI<xiiVariant>())
      continue;

    xiiHybridArray<xiiPropertySelection, 1> selection;
    selection.PushBack({pObject, xiiVariant()});
    if (property->GetCategory() == xiiPropertyCategory::Member)
    {
      if (bInsidePrefab)
      {
        xiiDefaultObjectState defaults(type, GetObjectAccessor(), selection.GetArrayPtr());
        if (defaults.GetStateProviderName() == "Prefab" && defaults.IsDefaultValue(property))
          continue;
      }
      AddValueReferences(pObject->GetTypeAccessor().GetValue(property->GetPropertyName()), flags, *pInfo);
    }
    else if (property->GetCategory() == xiiPropertyCategory::Array || property->GetCategory() == xiiPropertyCategory::Set || property->GetCategory() == xiiPropertyCategory::Map)
    {
      // One path for all collection categories preserves the original prefab-default semantics
      // without copying containers or accidentally inspecting a map's dictionary as its value.
      xiiHybridArray<xiiVariant, 16> keys;
      if (!pObject->GetTypeAccessor().GetKeys(property->GetPropertyName(), keys))
        continue;
      auto addElements = [&](const xiiDefaultContainerState* defaults) {
        for (const auto& key : keys)
        {
          if (defaults && defaults->GetStateProviderName() == "Prefab" && defaults->IsDefaultElement(key))
            continue;
          auto elementFlags = flags;
          if (key.IsA<xiiString>() || key.IsA<xiiStringView>())
          {
            if (const auto* parameter = parameterFlags.GetValue(key.ConvertTo<xiiString>()))
              elementFlags |= *parameter;
          }
          AddValueReferences(pObject->GetTypeAccessor().GetValue(property->GetPropertyName(), key), elementFlags, *pInfo);
        }
      };
      if (bInsidePrefab)
      {
        xiiDefaultContainerState defaults(type, GetObjectAccessor(), selection.GetArrayPtr(), property->GetPropertyName());
        addElements(&defaults);
      }
      else
        addElements(nullptr);
    }
  }

  for (const auto* child : pObject->GetChildren())
  {
    if (child->GetParentPropertyType()->GetAttributeByType<xiiTemporaryAttribute>() == nullptr)
      AddReferences(child, pInfo, bInsidePrefab);
  }
}

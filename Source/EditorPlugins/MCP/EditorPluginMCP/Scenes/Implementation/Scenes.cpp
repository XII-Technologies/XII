/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <Core/World/World.h>
#include <EditorPluginMCP/EditorMCP.h>
#include <EditorPluginMCP/EditorMCPArguments.h>
#include <Foundation/Types/ScopeExit.h>
#include <MCPPlugin/Reflection.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

using namespace xiiEditorMCPArguments;

namespace
{
  xiiVariantDictionary DescribeDocumentObject(const xiiDocumentObject& object)
  {
    xiiVariantDictionary row;
    row["id"]             = FormatGuid(object.GetGuid());
    row["type"]           = xiiString(object.GetType()->GetTypeName());
    row["parentProperty"] = xiiString(object.GetParentProperty());
    row["childCount"]     = object.GetChildren().GetCount();
    return row;
  }

  const xiiDocumentObject* FindDocumentObject(xiiObjectAccessorBase& ref_accessor, const xiiVariantDictionary& arguments, xiiStringView sKey)
  {
    const auto id = ReadGuid(arguments, sKey);
    return id.IsValid() ? ref_accessor.GetObject(id) : nullptr;
  }

  xiiStatus ApplySceneCommand(xiiObjectAccessorBase& ref_accessor, const xiiVariantDictionary& operation, xiiVariantDictionary& out_row)
  {
    const auto  sAction = xiiMCPJson::GetString(operation, "action");
    const auto* pObject = FindDocumentObject(ref_accessor, operation, "object");
    if (sAction == "addObject" || sAction == "addComponent" || sAction == "reparent")
    {
      const auto* pParent    = FindDocumentObject(ref_accessor, operation, "parent");
      const bool  bComponent = sAction == "addComponent";
      if ((operation.Contains("parent") && pParent == nullptr) ||
          (pParent != nullptr && !pParent->GetType()->IsDerivedFrom<xiiGameObject>()) ||
          (bComponent && pParent == nullptr))
      {
        return xiiStatus("The destination must be a valid game object. A component requires a parent.");
      }
      const auto sProperty = pParent == nullptr ? "" : bComponent ? "Components" :
                                                                    "Children";
      if (sAction == "reparent")
      {
        if (pObject == nullptr || !pObject->GetType()->IsDerivedFrom<xiiGameObject>())
        {
          return xiiStatus("Only a game object can be reparented.");
        }
        return ref_accessor.MoveObjectByName(pObject, pParent, sProperty, xiiInt32(-1));
      }

      const xiiRTTI* pType = xiiGetStaticRTTI<xiiGameObject>();
      if (bComponent)
      {
        pType = xiiRTTI::FindTypeByName(xiiMCPJson::GetString(operation, "type"));
        if (pType == nullptr || !pType->IsDerivedFrom<xiiComponent>() || pType->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
        {
          return xiiStatus("Provide a concrete component type.");
        }
      }
      auto id = ReadGuid(operation, "newId");
      if (operation.Contains("newId") && (!id.IsValid() || ref_accessor.GetObject(id) != nullptr))
      {
        return xiiStatus("The requested new object UUID is invalid or already in use.");
      }
      const auto status = ref_accessor.AddObjectByName(pParent, sProperty, xiiInt32(-1), pType, id);
      if (status.Succeeded())
      {
        out_row["id"] = FormatGuid(id);
      }
      return status;
    }

    if (pObject == nullptr || (!pObject->GetType()->IsDerivedFrom<xiiGameObject>() && !pObject->GetType()->IsDerivedFrom<xiiComponent>()))
    {
      return xiiStatus("Select a game object or component in the open scene.");
    }
    if (sAction == "remove")
    {
      return ref_accessor.RemoveObject(pObject);
    }
    if (sAction == "set")
    {
      const auto  sProperty = xiiMCPJson::GetString(operation, "property");
      const auto* pProperty = pObject->GetType()->FindPropertyByName(sProperty);
      const auto* pValue    = operation.GetValue("value");
      xiiVariant  converted;
      if (pProperty == nullptr || pValue == nullptr || !xiiMCPReflection::ConvertArgument(*pProperty, *pValue, converted))
      {
        return xiiStatus("The property is unavailable, read-only, or the JSON value does not match its supported type.");
      }
      return ref_accessor.SetValue(pObject, pProperty, converted);
    }
    return xiiStatus("Unknown scene edit action.");
  }
} // namespace

bool xiiEditorMCP::IsScene(const xiiDocument& document)
{
  const auto sType = document.GetDocumentTypeName();
  return sType == "Scene" || sType == "Prefab" || sType == "Layer";
}

xiiStatus xiiEditorMCP::Scene(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  auto* pDocument = FindDocument(arguments);
  if (pDocument == nullptr || !IsScene(*pDocument))
  {
    return xiiStatus("Select an open scene, prefab or layer in the active project.");
  }
  return InspectDocumentObject(*pDocument, arguments, out_result);
}

xiiStatus xiiEditorMCP::InspectDocumentObject(xiiDocument& ref_document, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  auto&       accessor = *ref_document.GetObjectAccessor();
  const auto* pObject  = arguments.Contains("object") ? FindDocumentObject(accessor, arguments, "object") : accessor.GetObjectManager()->GetRootObject();
  xiiUInt64   uiOffset = 0, uiLimit = 0;
  if (pObject == nullptr || !ReadPage(arguments, uiOffset, uiLimit))
  {
    return xiiStatus("The requested object or page is unavailable.");
  }

  DescribeDocument(ref_document, GetDocumentSession(ref_document), out_result);
  out_result["object"]     = DescribeDocumentObject(*pObject);
  const auto&     children = pObject->GetChildren();
  xiiVariantArray rows;
  if (uiOffset < children.GetCount())
  {
    const auto uiEnd = xiiMath::Min(static_cast<xiiUInt64>(children.GetCount()), uiOffset + uiLimit);
    for (xiiUInt32 uiIndex = static_cast<xiiUInt32>(uiOffset); uiIndex < uiEnd; ++uiIndex)
    {
      rows.PushBack(DescribeDocumentObject(*children[uiIndex]));
    }
    if (uiEnd < children.GetCount())
    {
      out_result["nextOffset"] = uiEnd;
    }
  }
  out_result["children"] = std::move(rows);
  const auto& type       = *pObject->GetType();
  out_result["schema"]   = xiiMCPReflection::Describe(type);
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  type.GetAllProperties(properties);
  xiiVariantDictionary values;
  for (const auto* pProperty : properties)
  {
    if (pProperty->GetCategory() != xiiPropertyCategory::Member || pProperty->GetFlags().IsAnySet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner) ||
        pProperty->GetAttributeByType<xiiHiddenAttribute>() != nullptr || pProperty->GetAttributeByType<xiiTemporaryAttribute>() != nullptr || values.GetCount() >= 64)
    {
      continue;
    }
    xiiVariant value, json;
    if (accessor.GetValue(pObject, pProperty, value).Succeeded() && xiiMCPReflection::ConvertValue(value, json))
    {
      values[pProperty->GetPropertyName()] = std::move(json);
    }
  }
  out_result["properties"] = std::move(values);
  return XII_SUCCESS;
}

xiiStatus xiiEditorMCP::EditScene(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  xiiDocument* pDocument  = nullptr;
  const auto   validation = ValidateEdit(arguments, pDocument);
  if (validation.Failed())
  {
    return validation;
  }
  if (!IsScene(*pDocument))
  {
    return xiiStatus("Select an open writable scene, prefab or layer.");
  }
  const auto* pOperations = arguments.GetValue("operations");
  if (pOperations == nullptr || !pOperations->IsA<xiiVariantArray>())
  {
    return xiiStatus("Provide an array of scene edit operations.");
  }
  const auto& operations = pOperations->Get<xiiVariantArray>();
  if (operations.IsEmpty() || operations.GetCount() > 24)
  {
    return xiiStatus("Submit between 1 and 24 operations per transaction.");
  }

  auto& accessor = *pDocument->GetObjectAccessor();
  accessor.StartTransaction("Apply MCP scene edits");
  bool bCommitted = false;
  XII_SCOPE_EXIT(if (!bCommitted) { accessor.CancelTransaction(); });
  xiiVariantArray results;
  for (const auto& operation : operations)
  {
    if (context.HasExpired() || !operation.IsA<xiiVariantDictionary>())
    {
      return xiiStatus("The batch expired or contains an invalid operation.");
    }
    xiiVariantDictionary row;
    const auto           status = ApplySceneCommand(accessor, operation.Get<xiiVariantDictionary>(), row);
    if (status.Failed())
    {
      return xiiStatus(xiiFmt("Operation {0} failed: {1}", results.GetCount(), status.GetMessageString()));
    }
    results.PushBack(std::move(row));
  }
  if (context.HasExpired())
  {
    return xiiStatus("The batch deadline expired before commit.");
  }
  accessor.FinishTransaction();
  bCommitted = true;
  DescribeDocument(*pDocument, GetDocumentSession(*pDocument), out_result);
  out_result["operations"] = std::move(results);
  return XII_SUCCESS;
}

xiiResult xiiEditorMCP::RegisterScenes()
{
  const auto read      = xiiMCPPermission::Read;
  const auto edit      = xiiMCPPermission::Edit;
  const auto sDocument = R"({"type":"object","required":["document"],"properties":{"document":{"type":"string"},"object":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":128}}})";
  if (m_Registry.RegisterTool("editor.scene", "Browse children of a scene, prefab or layer object and inspect its reflected members. Omit object for the root.", sDocument, read, xiiMakeDelegate(&xiiEditorMCP::Scene, this)).Failed() ||
      m_Registry.RegisterTool("editor.edit_scene", "Apply a bounded batch in one undoable transaction. Set, addObject, addComponent, remove and reparent are supported.", R"({"type":"object","required":["document","session","revision","generation","operations"],"properties":{"document":{"type":"string"},"session":{"type":"string"},"revision":{"type":"integer"},"generation":{"type":"integer"},"operations":{"type":"array","minItems":1,"maxItems":24,"items":{"type":"object","required":["action"],"properties":{"action":{"enum":["addObject","addComponent","set","remove","reparent"]},"object":{"type":"string"},"newId":{"type":"string"},"parent":{"type":"string"},"type":{"type":"string"},"property":{"type":"string"},"value":{}}}}}})", edit, xiiMakeDelegate(&xiiEditorMCP::EditScene, this)).Failed())
  {
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

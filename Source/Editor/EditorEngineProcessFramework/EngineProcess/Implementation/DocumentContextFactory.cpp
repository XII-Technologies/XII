/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>

xiiStatus xiiEngineProcessDocumentContext::AllocateDocumentContext(const xiiDocumentOpenMsgToEngine& request, xiiEngineProcessDocumentContext*& out_pContext)
{
  out_pContext = nullptr;
  if (!request.m_bDocumentOpen || !request.m_DocumentGuid.IsValid() || request.m_sDocumentType.IsEmpty() || request.m_sDocumentType.FindSubString(";") != nullptr)
    return xiiStatus("A document open request requires a valid GUID and a single nonempty document type.");

  const xiiRTTI*         pContextType   = nullptr;
  const xiiRTTI*         pDuplicateType = nullptr;
  const xiiStringBuilder requestedType(";", request.m_sDocumentType, ";");
  xiiRTTI::ForEachDerivedType<xiiEngineProcessDocumentContext>([&](const xiiRTTI* pType) {
    const auto* pProperty = pType->FindPropertyByName("DocumentType");
    if (!pProperty || pProperty->GetCategory() != xiiPropertyCategory::Constant)
      return;

    const auto& value = static_cast<const xiiAbstractConstantProperty*>(pProperty)->GetConstant();
    if (!value.CanConvertTo<xiiString>())
      return;
    const xiiStringBuilder documentTypes(";", value.ConvertTo<xiiString>(), ";");
    if (documentTypes.FindSubString(requestedType) == nullptr)
      return;

    if (pContextType)
      pDuplicateType = pType;
    else
      pContextType = pType;
  });

  if (!pContextType)
    return xiiStatus(xiiFmt("No engine context is registered for document type '{}'.", request.m_sDocumentType));
  if (pDuplicateType)
    return xiiStatus(xiiFmt("Document type '{}' is registered by both '{}' and '{}'.", request.m_sDocumentType, pContextType->GetTypeName(), pDuplicateType->GetTypeName()));

  const xiiAbstractFunctionProperty* pAllocator = nullptr;
  for (const auto* pFunction : pContextType->GetFunctions())
  {
    if (pFunction->GetPropertyName() != "AllocateContext")
      continue;
    if (pAllocator)
      return xiiStatus(xiiFmt("Context '{}' registers multiple AllocateContext functions.", pContextType->GetTypeName()));
    pAllocator = pFunction;
  }

  if (pAllocator)
  {
    if (pAllocator->GetFunctionType() != xiiFunctionType::StaticMember ||
        pAllocator->GetReturnType() != xiiGetStaticRTTI<xiiEngineProcessDocumentContext>() ||
        pAllocator->GetReturnFlags().IsSet(xiiPropertyFlags::Pointer) == false ||
        pAllocator->GetArgumentCount() != 1 ||
        pAllocator->GetArgumentType(0) != xiiGetStaticRTTI<xiiDocumentOpenMsgToEngine>() ||
        !pAllocator->GetArgumentFlags(0).AreAllSet(xiiPropertyFlags::Pointer | xiiPropertyFlags::Const))
      return xiiStatus(xiiFmt("Context '{}' requires a static xiiEngineProcessDocumentContext* AllocateContext(const xiiDocumentOpenMsgToEngine*).", pContextType->GetTypeName()));

    xiiVariant                    result;
    xiiHybridArray<xiiVariant, 1> arguments;
    arguments.PushBack(&request);
    pAllocator->Execute(nullptr, arguments, result);
    if (!result.IsA<xiiEngineProcessDocumentContext*>())
      return xiiStatus(xiiFmt("Custom allocator for '{}' returned an invalid context result.", request.m_sDocumentType));
    out_pContext = result.Get<xiiEngineProcessDocumentContext*>();
  }
  else
  {
    if (!pContextType->GetAllocator()->CanAllocate())
      return xiiStatus(xiiFmt("Context '{}' cannot be allocated.", pContextType->GetTypeName()));
    out_pContext = pContextType->GetAllocator()->Allocate<xiiEngineProcessDocumentContext>();
  }

  if (!out_pContext)
    return xiiStatus(xiiFmt("Context allocation rejected document type '{}'.", request.m_sDocumentType));
  return XII_SUCCESS;
}

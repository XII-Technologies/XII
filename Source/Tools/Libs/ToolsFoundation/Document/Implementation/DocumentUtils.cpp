#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/DocumentUtils.h>

xiiStatus xiiDocumentUtils::IsValidSaveLocationForDocument(const char* szDocument, const xiiDocumentTypeDescriptor** out_pTypeDesc)
{
  const xiiDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (xiiDocumentManager::FindDocumentTypeFromPath(szDocument, true, pTypeDesc).Failed())
  {
    xiiStringBuilder sTemp;
    sTemp.Format("The selected file extension '{0}' is not registered with any known type.\nCannot create file '{1}'",
                 xiiPathUtils::GetFileExtension(szDocument), szDocument);
    return xiiStatus(sTemp.GetData());
  }

  if (xiiDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(szDocument))
  {
    return xiiStatus("The selected document is already open. You need to close the document before you can re-create it.");
  }

  if (out_pTypeDesc)
  {
    *out_pTypeDesc = pTypeDesc;
  }
  return xiiStatus(XII_SUCCESS);
}

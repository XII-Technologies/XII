#pragma once

#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiDocumentObject;
struct xiiDocumentTypeDescriptor;

class XII_TOOLSFOUNDATION_DLL xiiDocumentUtils
{
public:
  static xiiStatus IsValidSaveLocationForDocument(xiiStringView sDocument, const xiiDocumentTypeDescriptor** out_pTypeDesc = nullptr);
};

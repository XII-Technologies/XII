/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectVisitor.h>

xiiDocumentObjectVisitor::xiiDocumentObjectVisitor(const xiiDocumentObjectManager* pManager, xiiStringView sChildrenProperty /*= "Children"*/, xiiStringView sRootProperty /*= "Children"*/) :
  m_pManager(pManager), m_sChildrenProperty(sChildrenProperty), m_sRootProperty(sRootProperty)
{
  const xiiAbstractProperty* pRootProp = m_pManager->GetRootObject()->GetType()->FindPropertyByName(sRootProperty);
  XII_ASSERT_DEV(pRootProp, "Given root property '{0}' does not exist on root object", sRootProperty);
  XII_ASSERT_DEV(pRootProp->GetCategory() == xiiPropertyCategory::Set || pRootProp->GetCategory() == xiiPropertyCategory::Array, "Traverser only works on arrays and sets.");

  // const xiiAbstractProperty* pChildProp = pRootProp->GetSpecificType()->FindPropertyByName(szChildrenProperty);
  // XII_ASSERT_DEV(pChildProp, "Given child property '{0}' does not exist", szChildrenProperty);
  // XII_ASSERT_DEV(pChildProp->GetCategory() == xiiPropertyCategory::Set || pRootProp->GetCategory() == xiiPropertyCategory::Array, "Traverser only works on arrays and sets.");
}

void xiiDocumentObjectVisitor::Visit(const xiiDocumentObject* pObject, bool bVisitStart, VisitorFunction function)
{
  xiiStringView sProperty = m_sChildrenProperty;
  if (pObject == nullptr || pObject == m_pManager->GetRootObject())
  {
    pObject   = m_pManager->GetRootObject();
    sProperty = m_sRootProperty;
  }

  if (!bVisitStart || function(pObject))
  {
    TraverseChildren(pObject, sProperty, function);
  }
}

void xiiDocumentObjectVisitor::TraverseChildren(const xiiDocumentObject* pObject, xiiStringView sProperty, VisitorFunction& function)
{
  const xiiInt32 iChildren = pObject->GetTypeAccessor().GetCount(sProperty);
  for (xiiInt32 i = 0; i < iChildren; i++)
  {
    xiiVariant obj = pObject->GetTypeAccessor().GetValue(sProperty, i);
    XII_ASSERT_DEBUG(obj.IsValid() && obj.IsA<xiiUuid>(), "null obj found during traversal.");
    const xiiDocumentObject* pChild = m_pManager->GetObject(obj.Get<xiiUuid>());
    if (function(pChild))
    {
      TraverseChildren(pChild, m_sChildrenProperty, function);
    }
  }
}

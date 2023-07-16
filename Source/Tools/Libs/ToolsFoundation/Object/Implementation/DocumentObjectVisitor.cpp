#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectVisitor.h>

xiiDocumentObjectVisitor::xiiDocumentObjectVisitor(
  const xiiDocumentObjectManager* pManager,
  const char*                     szChildrenProperty /*= "Children"*/,
  const char*                     szRootProperty /*= "Children"*/) :
  m_pManager(pManager), m_sChildrenProperty(szChildrenProperty), m_sRootProperty(szRootProperty)
{
  const xiiAbstractProperty* pRootProp = m_pManager->GetRootObject()->GetType()->FindPropertyByName(szRootProperty);
  XII_ASSERT_DEV(pRootProp, "Given root property '{0}' does not exist on root object", szRootProperty);
  XII_ASSERT_DEV(pRootProp->GetCategory() == xiiPropertyCategory::Set || pRootProp->GetCategory() == xiiPropertyCategory::Array,
                 "Traverser only works on arrays and sets.");

  // const xiiAbstractProperty* pChildProp = pRootProp->GetSpecificType()->FindPropertyByName(szChildrenProperty);
  // XII_ASSERT_DEV(pChildProp, "Given child property '{0}' does not exist", szChildrenProperty);
  // XII_ASSERT_DEV(pChildProp->GetCategory() == xiiPropertyCategory::Set || pRootProp->GetCategory() == xiiPropertyCategory::Array, "Traverser
  // only works on arrays and sets.");
}

void xiiDocumentObjectVisitor::Visit(const xiiDocumentObject* pObject, bool bVisitStart, VisitorFunction function)
{
  const char* szProperty = m_sChildrenProperty;
  if (pObject == nullptr || pObject == m_pManager->GetRootObject())
  {
    pObject    = m_pManager->GetRootObject();
    szProperty = m_sRootProperty;
  }

  if (!bVisitStart || function(pObject))
  {
    TraverseChildren(pObject, szProperty, function);
  }
}

void xiiDocumentObjectVisitor::TraverseChildren(const xiiDocumentObject* pObject, const char* szProperty, VisitorFunction& function)
{
  const xiiInt32 iChildren = pObject->GetTypeAccessor().GetCount(szProperty);
  for (xiiInt32 i = 0; i < iChildren; i++)
  {
    xiiVariant obj = pObject->GetTypeAccessor().GetValue(szProperty, i);
    XII_ASSERT_DEBUG(obj.IsValid() && obj.IsA<xiiUuid>(), "null obj found during traversal.");
    const xiiDocumentObject* pChild = m_pManager->GetObject(obj.Get<xiiUuid>());
    if (function(pChild))
    {
      TraverseChildren(pChild, m_sChildrenProperty, function);
    }
  }
}

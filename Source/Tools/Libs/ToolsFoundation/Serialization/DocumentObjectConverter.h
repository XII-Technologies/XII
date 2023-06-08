#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiObjectAccessorBase;

/// \brief Writes the state of an xiiDocumentObject to an abstract graph.
///
/// This information can then be applied to another xiiDocument object through xiiDocumentObjectConverterReader,
/// or to entirely different class using xiiRttiConverterReader.
class XII_TOOLSFOUNDATION_DLL xiiDocumentObjectConverterWriter
{
public:
  using FilterFunction = xiiDelegate<bool(const xiiDocumentObject*, const xiiAbstractProperty*)>;
  xiiDocumentObjectConverterWriter(xiiAbstractObjectGraph* pGraph, const xiiDocumentObjectManager* pManager, FilterFunction filter = FilterFunction())
  {
    m_pGraph   = pGraph;
    m_pManager = pManager;
    m_Filter   = filter;
  }

  xiiAbstractObjectNode* AddObjectToGraph(const xiiDocumentObject* pObject, const char* szNodeName = nullptr);

private:
  void AddProperty(xiiAbstractObjectNode* pNode, const xiiAbstractProperty* pProp, const xiiDocumentObject* pObject);
  void AddProperties(xiiAbstractObjectNode* pNode, const xiiDocumentObject* pObject);

  xiiAbstractObjectNode* AddSubObjectToGraph(const xiiDocumentObject* pObject, const char* szNodeName);

  const xiiDocumentObjectManager*  m_pManager;
  xiiAbstractObjectGraph*          m_pGraph;
  FilterFunction                   m_Filter;
  xiiSet<const xiiDocumentObject*> m_QueuedObjects;
};


class XII_TOOLSFOUNDATION_DLL xiiDocumentObjectConverterReader
{
public:
  enum class Mode
  {
    CreateOnly,
    CreateAndAddToDocument,
  };
  xiiDocumentObjectConverterReader(const xiiAbstractObjectGraph* pGraph, xiiDocumentObjectManager* pManager, Mode mode);

  xiiDocumentObject* CreateObjectFromNode(const xiiAbstractObjectNode* pNode);
  void               ApplyPropertiesToObject(const xiiAbstractObjectNode* pNode, xiiDocumentObject* pObject);

  xiiUInt32                GetNumUnknownObjectCreations() const { return m_uiUnknownTypeInstances; }
  const xiiSet<xiiString>& GetUnknownObjectTypes() const { return m_UnknownTypes; }

  static void ApplyDiffToObject(xiiObjectAccessorBase* pObjectAccessor, const xiiDocumentObject* pObject, xiiDeque<xiiAbstractGraphDiffOperation>& ref_diff);

private:
  void        AddObject(xiiDocumentObject* pObject, xiiDocumentObject* pParent, const char* szParentProperty, xiiVariant index);
  void        ApplyProperty(xiiDocumentObject* pObject, xiiAbstractProperty* pProp, const xiiAbstractObjectNode::Property* pSource);
  static void ApplyDiff(xiiObjectAccessorBase* pObjectAccessor, const xiiDocumentObject* pObject, xiiAbstractProperty* pProp, xiiAbstractGraphDiffOperation& op, xiiDeque<xiiAbstractGraphDiffOperation>& diff);

  Mode                          m_Mode;
  xiiDocumentObjectManager*     m_pManager;
  const xiiAbstractObjectGraph* m_pGraph;
  xiiSet<xiiString>             m_UnknownTypes;
  xiiUInt32                     m_uiUnknownTypeInstances;
};

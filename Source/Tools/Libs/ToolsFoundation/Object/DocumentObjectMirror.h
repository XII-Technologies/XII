/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>


/// An object change starts at the heap object m_Root (because we can only safely store pointers to those).
///  From this object we follow m_Steps (member arrays, structs) to execute m_Change at the end target.
///
/// In case of an NodeAdded operation, m_GraphData contains the entire subgraph of this node.
class XII_TOOLSFOUNDATION_DLL xiiObjectChange
{
public:
  xiiObjectChange() = default;
  xiiObjectChange(const xiiObjectChange&);
  xiiObjectChange(xiiObjectChange&& rhs);
  void operator=(xiiObjectChange&& rhs);
  void operator=(xiiObjectChange& rhs);
  void GetGraph(xiiAbstractObjectGraph& ref_graph) const;
  void SetGraph(xiiAbstractObjectGraph& ref_graph);

  xiiUuid                                m_Root;      //< The object that is the parent of the op, namely the parent heap object we can store a pointer to.
  xiiHybridArray<xiiPropertyPathStep, 2> m_Steps;     //< Path from root to target of change.
  xiiDiffOperation                       m_Change;    //< Change at the target.
  xiiDataBuffer                          m_GraphData; //< In case of ObjectAdded, this holds the binary serialized object graph.
};
XII_DECLARE_REFLECTABLE_TYPE(XII_TOOLSFOUNDATION_DLL, xiiObjectChange);


class XII_TOOLSFOUNDATION_DLL xiiDocumentObjectMirror
{
public:
  xiiDocumentObjectMirror();
  virtual ~xiiDocumentObjectMirror();

  void InitSender(const xiiDocumentObjectManager* pManager);
  void InitReceiver(xiiRttiConverterContext* pContext);
  void DeInit();

  using FilterFunction = xiiDelegate<bool(const xiiDocumentObject*, xiiStringView)>;
  /// \brief
  ///
  /// \param filter
  ///   Filter that defines whether an object property should be mirrored or not.
  void SetFilterFunction(FilterFunction filter);

  void SendDocument();
  void Clear();

  void TreeStructureEventHandler(const xiiDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);

  void*       GetNativeObjectPointer(const xiiDocumentObject* pObject);
  const void* GetNativeObjectPointer(const xiiDocumentObject* pObject) const;

protected:
  bool           IsRootObject(const xiiDocumentObject* pParent);
  bool           IsHeapAllocated(const xiiDocumentObject* pParent, xiiStringView sParentProperty);
  bool           IsDiscardedByFilter(const xiiDocumentObject* pObject, xiiStringView sProperty) const;
  static void    CreatePath(xiiObjectChange& out_change, const xiiDocumentObject* pRoot, xiiStringView sProperty);
  static xiiUuid FindRootOpObject(const xiiDocumentObject* pObject, xiiHybridArray<const xiiDocumentObject*, 8>& path);
  static void    FlattenSteps(const xiiArrayPtr<const xiiDocumentObject* const> path, xiiHybridArray<xiiPropertyPathStep, 2>& out_steps);

  virtual void ApplyOp(xiiObjectChange& change);
  void         ApplyOp(xiiRttiConverterObject object, const xiiObjectChange& change);

protected:
  xiiRttiConverterContext*        m_pContext;
  const xiiDocumentObjectManager* m_pManager;
  FilterFunction                  m_Filter;
};

/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Delegate.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiDocumentObjectManager;
class xiiDocumentObject;

/// Implements visitor pattern for content of the document object manager.
class XII_TOOLSFOUNDATION_DLL xiiDocumentObjectVisitor
{
public:
  /// Constructor
  ///
  /// \param pManager
  ///   Manager that will be iterated through.
  /// \param szChildrenProperty
  ///   Name of the property that is used for finding children on an object.
  /// \param szRootProperty
  ///   Same as szChildrenProperty, but for the root object of the document.
  xiiDocumentObjectVisitor(const xiiDocumentObjectManager* pManager, xiiStringView sChildrenProperty = "Children", xiiStringView sRootProperty = "Children");

  using VisitorFunction = xiiDelegate<bool(const xiiDocumentObject*)>;
  /// Executes depth first traversal starting at the given node.
  ///
  /// \param pObject
  ///   Object to start traversal at.
  /// \param bVisitStart
  ///   If true, function will be executed for the start object as well.
  /// \param function
  ///   Functions executed for each visited object. Should true if the object's children should be traversed.
  void Visit(const xiiDocumentObject* pObject, bool bVisitStart, VisitorFunction function);

private:
  void TraverseChildren(const xiiDocumentObject* pObject, xiiStringView sProperty, VisitorFunction& function);

  const xiiDocumentObjectManager* m_pManager = nullptr;
  xiiString                       m_sChildrenProperty;
  xiiString                       m_sRootProperty;
};

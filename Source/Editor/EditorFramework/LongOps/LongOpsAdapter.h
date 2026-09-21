/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashSet.h>
#include <ToolsFoundation/Document/DocumentManager.h>

struct xiiDocumentObjectStructureEvent;
struct xiiPhantomRttiManagerEvent;
class xiiRTTI;

/// This singleton lives in the editor process and monitors all xiiSceneDocument's for components with the xiiLongOpAttribute.
///
/// All such components will be automatically registered in the xiiLongOpControllerManager, such that their functionality
/// is exposed to the user.
///
/// Since this class adapts the components with the xiiLongOpAttribute to the xiiLongOpControllerManager, it does not have any public
/// functionality.
class xiiLongOpsAdapter
{
  XII_DECLARE_SINGLETON(xiiLongOpsAdapter);

public:
  xiiLongOpsAdapter();
  ~xiiLongOpsAdapter();

private:
  void DocumentManagerEventHandler(const xiiDocumentManager::Event& e);
  void StructureEventHandler(const xiiDocumentObjectStructureEvent& e);
  void PhantomTypeRegistryEventHandler(const xiiPhantomRttiManagerEvent& e);
  void CheckAllTypes();
  void ObjectAdded(const xiiDocumentObject* pObject);
  void ObjectRemoved(const xiiDocumentObject* pObject);

  xiiHashSet<const xiiRTTI*> m_TypesWithLongOps;
};

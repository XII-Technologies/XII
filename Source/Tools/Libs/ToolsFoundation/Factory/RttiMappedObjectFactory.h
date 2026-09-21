/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// A factory that creates the closest matching objects according to the passed type.
///
/// Creators can be registered at the factory for a specific type.
/// When the create function is called for a type, the parent type hierarchy is traversed until
/// the first type is found for which a creator is registered.
template <typename Object>
class xiiRttiMappedObjectFactory
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRttiMappedObjectFactory);

public:
  xiiRttiMappedObjectFactory();
  ~xiiRttiMappedObjectFactory();

  using CreateObjectFunc = Object* (*)(const xiiRTTI*);

  void    RegisterCreator(const xiiRTTI* pType, CreateObjectFunc creator);
  void    UnregisterCreator(const xiiRTTI* pType);
  Object* CreateObject(const xiiRTTI* pType);

  struct Event
  {
    enum class Type
    {
      CreatorAdded,
      CreatorRemoved
    };

    Type           m_Type;
    const xiiRTTI* m_pRttiType;
  };

  xiiEvent<const Event&> m_Events;

private:
  xiiHashTable<const xiiRTTI*, CreateObjectFunc> m_Creators;
};

#include <ToolsFoundation/Factory/Implementation/RttiMappedObjectFactory_inl.h>

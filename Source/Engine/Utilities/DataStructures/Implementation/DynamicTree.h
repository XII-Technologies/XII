/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Math/Vec3.h>
#include <Utilities/UtilitiesDLL.h>

struct xiiDynamicTree
{
  struct xiiObjectData
  {
    xiiInt32 m_iObjectType;
    xiiInt32 m_iObjectInstance;
  };

  struct xiiMultiMapKey
  {
    xiiUInt32 m_uiKey;
    xiiUInt32 m_uiCounter;

    xiiMultiMapKey()
    {
      m_uiKey     = 0;
      m_uiCounter = 0;
    }

    inline bool operator<(const xiiMultiMapKey& rhs) const
    {
      if (m_uiKey == rhs.m_uiKey)
        return m_uiCounter < rhs.m_uiCounter;

      return m_uiKey < rhs.m_uiKey;
    }

    inline bool operator==(const xiiMultiMapKey& rhs) const { return (m_uiCounter == rhs.m_uiCounter && m_uiKey == rhs.m_uiKey); }
  };
};

using xiiDynamicTreeObject      = xiiMap<xiiDynamicTree::xiiMultiMapKey, xiiDynamicTree::xiiObjectData>::Iterator;
using xiiDynamicTreeObjectConst = xiiMap<xiiDynamicTree::xiiMultiMapKey, xiiDynamicTree::xiiObjectData>::ConstIterator;

/// Callback type for object queries. Return "false" to abort a search (e.g. when the desired element has been found).
using XII_VISIBLE_OBJ_CALLBACK = bool (*)(void*, xiiDynamicTreeObjectConst);

class xiiDynamicOctree;
class xiiDynamicQuadtree;

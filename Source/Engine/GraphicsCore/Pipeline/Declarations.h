#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

using xiiViewId = xiiGenericId<24, 8>;

class xiiViewHandle
{
 XII_DECLARE_HANDLE_TYPE(xiiViewHandle, xiiViewId);

  friend class xiiRenderWorldModule;
};

/// \brief HashHelper implementation so view handles can be used as key in a hashtable.
template <>
struct xiiHashHelper<xiiViewHandle>
{
 XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiViewHandle value) { return value.GetInternalID().m_Data * 2654435761U; }

 XII_ALWAYS_INLINE static bool Equal(xiiViewHandle a, xiiViewHandle b) { return a == b; }
};

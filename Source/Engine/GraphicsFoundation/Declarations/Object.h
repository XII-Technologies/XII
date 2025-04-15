#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

/// \brief Base Graphics Abstraction Layer Object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALObject : public xiiReflectedClass, public xiiRefCounted
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALObject, xiiReflectedClass);

public:
  /// \brief Returns the debug name of this resource.
  [[nodiscard]] XII_ALWAYS_INLINE xiiStringView GetDebugName() const { return m_sDebugName.GetView(); }

  /// \brief Sets the debug name for this resource.
  void SetDebugName(xiiStringView sDebugName);

protected:
  friend class xiiGALDevice;

  XII_ALWAYS_INLINE virtual void SetDebugNamePlatform(xiiStringView sName) { XII_IGNORE_UNUSED(sName); };

private:
  mutable xiiHashedString m_sDebugName;
};

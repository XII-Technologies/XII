/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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
  void SetDebugName(xiiStringView sDebugName) const;

protected:
  friend class xiiMemoryUtils;

  xiiGALObject();
  virtual ~xiiGALObject();

  XII_ALWAYS_INLINE virtual void SetDebugNamePlatform(xiiStringView sName) const { XII_IGNORE_UNUSED(sName); };

private:
  mutable xiiHashedString m_sDebugName;
};

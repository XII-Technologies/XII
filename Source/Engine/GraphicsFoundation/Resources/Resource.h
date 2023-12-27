#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief Base Graphics Abstraction Layer Object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALResourceBase : public xiiReflectedClass, xiiRefCounted
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALResourceBase, xiiReflectedClass);

public:
  /// \brief Returns the xiiGALResourceBase pointer to this resource.
  XII_NODISCARD virtual xiiGALResourceBase* GetParentResource();

  /// \brief Returns the xiiGALDevice that created this resource.
  ///
  /// \note This does **not** increase the ref count on the device.
  XII_NODISCARD xiiGALDevice* GetDevice() const;

  /// \brief Returns the debug name of this resource.
  XII_NODISCARD xiiStringView GetDebugName() const;

  /// \brief Sets the debug name for this resource.
  virtual void SetDebugName(xiiStringView sDebugName);

protected:
  xiiGALResourceBase(xiiGALDevice* pDevice, xiiStringView sDebugName = {});

  friend class xiiGALDevice;

  xiiGALDevice* m_pDevice = nullptr;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  mutable xiiHashedString m_sDebugName;
#endif
};

#include <GraphicsFoundation/Resources/Implementation/Resource_inl.h>

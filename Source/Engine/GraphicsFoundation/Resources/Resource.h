#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief Base GAL object interface.
class XII_GRAPHICSFOUNDATION_DLL xiiGALResourceBase : public xiiRefCounted
{
public:
  virtual const xiiGALResourceBase* GetParentResource() const { return this; }

protected:
  friend class xiiGALDevice;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  mutable xiiHashedString m_sDebugName;
#endif
};

/// \brief Base class for GAL resources, stores a creation description of the object and also allows for reference counting.
template <typename CreationDescription>
class xiiGALResource : public xiiGALResourceBase
{
public:
  XII_ALWAYS_INLINE xiiGALResource(const CreationDescription& description) :
    m_Description(description)
  {
  }

  XII_ALWAYS_INLINE const CreationDescription& GetDescription() const { return m_Description; }

protected:
  CreationDescription m_Description;
};

#include <GraphicsFoundation/Resources/Implementation/Resource_inl.h>

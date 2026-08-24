/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief A base class for user-defined data assets.
///
/// Allows users to define their own asset types that can be created, edited and referenced in the editor without writing an editor plugin.
///
/// In order to do that, subclass xiiCustomData,
/// and put the macro XII_DECLARE_CUSTOM_DATA_RESOURCE(YourCustomData) into the header next to your custom type.
/// Also put the macro XII_DEFINE_CUSTOM_DATA_RESOURCE(YourCustomData) into the implementation file.
///
/// Those will also define resource and resource handle types, such as YourCustomDataResource and YourCustomDataResourceHandle.
///
/// For a full example see SampleCustomData in the SampleGamePlugin.
class XII_CORE_DLL xiiCustomData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCustomData, xiiReflectedClass);

public:
  /// \brief Loads the serialized custom data using a robust serialization-based method.
  ///
  /// This function does not need to be overridden. It will work, even if the properties change.
  /// It is only virtual in case you want to hook into the deserialization process.
  virtual void Load(class xiiAbstractObjectGraph& ref_graph, class xiiRttiConverterContext& ref_context, const class xiiAbstractObjectNode* pRootNode);
};

/// \brief Base class for resources that represent different implementations of xiiCustomData.
///
/// These resources are automatically generated using these macros:
///   XII_DECLARE_CUSTOM_DATA_RESOURCE(YourCustomData)
///   XII_DEFINE_CUSTOM_DATA_RESOURCE(YourCustomData)
///
/// Put the former into a header next to YourCustomData and the latter into an implementation file.
///
/// This builds these types:
///   YourCustomDataResource
///   YourCustomDataResourceHandle
///
/// You can then use these to reference this resource type for example in components.
/// For a full example search the SampleGamePlugin for SampleCustomDataResource and SampleCustomDataResourceHandle and see how they are used.
class XII_CORE_DLL xiiCustomDataResourceBase : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCustomDataResourceBase, xiiResource);

public:
  xiiCustomDataResourceBase();
  ~xiiCustomDataResourceBase();

protected:
  virtual void                CreateAndLoadData(xiiAbstractObjectGraph& ref_graph, xiiRttiConverterContext& ref_context, const xiiAbstractObjectNode* pRootNode) = 0;
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  xiiResourceLoadDescription         UpdateContent_Internal(xiiStreamReader* pStream, const xiiRTTI& rtti);
};

/// \brief Template resource type for sub-classed xiiCustomData types.
///
/// See xiiCustomDataResourceBase for details.
template <typename T>
class xiiCustomDataResource : public xiiCustomDataResourceBase
{
public:
  xiiCustomDataResource();
  ~xiiCustomDataResource();

  /// \brief Provides read access to the custom data type.
  ///
  /// Returns nullptr, if the resource wasn't loaded successfully.
  const T* GetData() const { return GetLoadingState() == xiiResourceState::Loaded ? reinterpret_cast<const T*>(m_Data) : nullptr; }

protected:
  virtual void CreateAndLoadData(xiiAbstractObjectGraph& graph, xiiRttiConverterContext& context, const xiiAbstractObjectNode* pRootNode) override;

  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;

  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;

  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  struct alignas(alignof(T))
  {
    xiiUInt8 m_Data[sizeof(T)];
  };
};

/// \brief Helper macro to declare a xiiCustomDataResource<T> and a matching resource handle
///
/// See xiiCustomDataResourceBase for details.
#define XII_DECLARE_CUSTOM_DATA_RESOURCE(SELF)                               \
  class SELF##Resource : public xiiCustomDataResource<SELF>                  \
  {                                                                          \
    XII_ADD_DYNAMIC_REFLECTION(SELF##Resource, xiiCustomDataResource<SELF>); \
    XII_RESOURCE_DECLARE_COMMON_CODE(SELF##Resource);                        \
  };                                                                         \
                                                                             \
  using SELF##ResourceHandle = xiiTypedResourceHandle<SELF##Resource>

/// \brief Helper macro to define a xiiCustomDataResource<T>
///
/// See xiiCustomDataResourceBase for details.
#define XII_DEFINE_CUSTOM_DATA_RESOURCE(SELF)                                                  \
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(SELF##Resource, 1, xiiRTTIDefaultAllocator<SELF##Resource>) \
  XII_END_DYNAMIC_REFLECTED_TYPE;                                                              \
                                                                                               \
  XII_RESOURCE_IMPLEMENT_COMMON_CODE(SELF##Resource)

#include <Core/Utils/Implementation/CustomData_inl.h>

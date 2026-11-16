/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Resources/Resource.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALResource, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALResourceView, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALResource::xiiGALResource(xiiSharedPtr<xiiGALDevice> pDevice) :
  xiiGALDeviceObject(std::move(pDevice))
{
}

xiiGALResource::~xiiGALResource() = default;

const xiiGALExternalMemoryDescription& xiiGALResource::GetExternalMemoryDescription() const
{
  static const xiiGALExternalMemoryDescription EmptyDescription;
  return EmptyDescription;
}

xiiGALResourceView::xiiGALResourceView(xiiSharedPtr<xiiGALDevice> pDevice) :
  xiiGALDeviceObject(std::move(pDevice))
{
}

xiiGALResourceView::~xiiGALResourceView() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Resource);

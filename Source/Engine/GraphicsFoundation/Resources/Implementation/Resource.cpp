#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Resource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALResource, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALResourceView, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALResource::xiiGALResource(xiiSharedPtr<xiiGALDevice> pDevice) :
  xiiGALDeviceObject(pDevice)
{
}

xiiGALResource::~xiiGALResource() = default;

xiiGALResourceView::xiiGALResourceView(xiiSharedPtr<xiiGALDevice> pDevice) :
  xiiGALDeviceObject(pDevice)
{
}

xiiGALResourceView::~xiiGALResourceView() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Resource);

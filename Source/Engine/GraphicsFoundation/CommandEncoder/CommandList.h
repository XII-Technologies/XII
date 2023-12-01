#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief This describes the command list creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandListCreationDescription : public xiiHashableStruct<xiiGALCommandListCreationDescription>
{
  XII_DECLARE_POD_TYPE();
};

/// \brief A command list interface. The command list has no methods. When a command list recording is finished, it is executed by the device context.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandList : public xiiGALObject<xiiGALCommandListCreationDescription>
{
public:
protected:
  friend class xiiGALDevice;

  xiiGALCommandList(const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandList();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALCommandList);

#include <GraphicsFoundation/CommandEncoder/Implementation/CommandList_inl.h>

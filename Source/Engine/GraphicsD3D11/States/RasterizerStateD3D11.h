#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/States/RasterizerState.h>

struct ID3D11RasterizerState;

class XII_GRAPHICSD3D11_DLL xiiGALRasterizerStateD3D11 final : public xiiGALRasterizerState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALRasterizerStateD3D11, xiiGALRasterizerState);

public:
  XII_ALWAYS_INLINE ID3D11RasterizerState* GetRasterizerState() const { return m_pRasterizerState; };

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALRasterizerStateCreationDescription& creationDescription);

  virtual ~xiiGALRasterizerStateD3D11();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

private:
  ID3D11RasterizerState* m_pRasterizerState = nullptr;
};

#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>

#include <d3d11.h>

xiiGALVertexDeclarationDX11::xiiGALVertexDeclarationDX11(const xiiGALVertexDeclarationCreationDescription& Description) :
  xiiGALVertexDeclaration(Description), m_pDXInputLayout(nullptr)
{
}

xiiGALVertexDeclarationDX11::~xiiGALVertexDeclarationDX11() = default;

static const char* GALSemanticToDX11[] = {"POSITION", "NORMAL", "TANGENT", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR",
                                          "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BITANGENT", "BONEINDICES",
                                          "BONEINDICES", "BONEWEIGHTS", "BONEWEIGHTS"};

static UINT GALSemanticToIndexDX11[] = {0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 1, 0, 1};

XII_CHECK_AT_COMPILETIME_MSG(XII_ARRAY_SIZE(GALSemanticToDX11) == xiiGALVertexAttributeSemantic::ENUM_COUNT,
                             "GALSemanticToDX11 array size does not match vertex attribute semantic count");
XII_CHECK_AT_COMPILETIME_MSG(XII_ARRAY_SIZE(GALSemanticToIndexDX11) == xiiGALVertexAttributeSemantic::ENUM_COUNT,
                             "GALSemanticToIndexDX11 array size does not match vertex attribute semantic count");

XII_DEFINE_AS_POD_TYPE(D3D11_INPUT_ELEMENT_DESC);

xiiResult xiiGALVertexDeclarationDX11::InitPlatform(xiiGALDevice* pDevice)
{
  xiiHybridArray<D3D11_INPUT_ELEMENT_DESC, 8> DXInputElementDescs;

  xiiGALDeviceDX11* pDXDevice = static_cast<xiiGALDeviceDX11*>(pDevice);

  const xiiGALShader* pShader = pDevice->GetShader(m_Description.m_hShader);

  if (pShader == nullptr || !pShader->GetDescription().HasByteCodeForStage(xiiGALShaderStage::VertexShader))
  {
    return XII_FAILURE;
  }

  // Copy attribute descriptions
  for (xiiUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); i++)
  {
    const xiiGALVertexAttribute& Current = m_Description.m_VertexAttributes[i];

    D3D11_INPUT_ELEMENT_DESC DXDesc;
    DXDesc.AlignedByteOffset = Current.m_uiOffset;
    DXDesc.Format            = pDXDevice->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType;

    if (DXDesc.Format == DXGI_FORMAT_UNKNOWN)
    {
      xiiLog::Error("Vertex attribute format {0} of attribute at index {1} is unknown!", Current.m_eFormat, i);
      return XII_FAILURE;
    }

    DXDesc.InputSlot            = Current.m_uiVertexBufferSlot;
    DXDesc.InputSlotClass       = Current.m_bInstanceData ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
    DXDesc.InstanceDataStepRate = Current.m_bInstanceData ? Current.m_uiStepRate : 0;
    DXDesc.SemanticIndex        = GALSemanticToIndexDX11[Current.m_eSemantic];
    DXDesc.SemanticName         = GALSemanticToDX11[Current.m_eSemantic];

    DXInputElementDescs.PushBack(DXDesc);
  }

  const xiiGALShaderDX11* pDXShader = static_cast<const xiiGALShaderDX11*>(pShader);
  if (FAILED(pDXDevice->GetDXDevice()->CreateInputLayout(
        &DXInputElementDescs[0], DXInputElementDescs.GetCount(), reinterpret_cast<const void*>(pDXShader->GetByteCode(xiiGALShaderStage::VertexShader).GetPtr()),
        pDXShader->GetByteCode(xiiGALShaderStage::VertexShader).GetCount(), &m_pDXInputLayout)))
  {
    return XII_FAILURE;
  }
  else
  {
    return XII_SUCCESS;
  }
}

xiiResult xiiGALVertexDeclarationDX11::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DX11_RELEASE(m_pDXInputLayout);
  return XII_SUCCESS;
}



XII_STATICLINK_FILE(RendererDX11, RendererDX11_Shader_Implementation_VertexDeclarationDX11);

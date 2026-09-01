/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Strings/StringConversion.h>
#include <Texture/Image/Conversions/DXTConversions.h>
#include <Texture/Image/Conversions/PixelConversions.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>
#include <Texture/Image/ImageConversion.h>

xiiCVarBool cvar_TexturePenalizeDXConversions("Texture.PenalizeDXConversions", false, xiiCVarFlags::RequiresRestart, "Add a penalty to DirectX-based conversion when choosing how to convert textures");

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  define XII_SUPPORTS_DIRECTXTEX XII_ON
#else
#  define XII_SUPPORTS_DIRECTXTEX XII_OFF
#endif

#if XII_ENABLED(XII_SUPPORTS_DIRECTXTEX)

#  ifdef DeleteFile
#    undef DeleteFile
#  endif

#  include <DirectXTex/DirectXTex.h>
#  include <d3d11.h>
#  include <dxgi.h>
#  include <dxgiformat.h>
#  include <wrl\client.h>

using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;

namespace
{
  bool GetDXGIFactory(IDXGIFactory1** pFactory)
  {
    if (!pFactory)
      return false;

    *pFactory = nullptr;

    using pfn_CreateDXGIFactory1 = HRESULT(WINAPI*)(REFIID riid, _Out_ void** ppFactory);

    static pfn_CreateDXGIFactory1 s_CreateDXGIFactory1 = nullptr;

    if (!s_CreateDXGIFactory1)
    {
      HMODULE hModDXGI = LoadLibraryA("dxgi.dll");
      if (!hModDXGI)
        return false;

      s_CreateDXGIFactory1 = reinterpret_cast<pfn_CreateDXGIFactory1>(reinterpret_cast<void*>(GetProcAddress(hModDXGI, "CreateDXGIFactory1")));
      if (!s_CreateDXGIFactory1)
        return false;
    }

    return SUCCEEDED(s_CreateDXGIFactory1(IID_PPV_ARGS(pFactory)));
  }

  enum class TypeOfDeviceCreated
  {
    None,
    Hardware,
    Software
  };

  /// Tries to create a hardware device, but falls back to a software device if there is one.
  TypeOfDeviceCreated CreateDevice(ComPtr<ID3D11Device>& ref_device)
  {
    ref_device = nullptr;

    // Find a hardware adapter if possible, otherwise find any adapter.
    ComPtr<IDXGIAdapter1> pHardwareAdapter1;
    ComPtr<IDXGIAdapter1> pFallbackAdapter1;
    {
      int                   adapter = 0;
      ComPtr<IDXGIFactory1> dxgiFactory;
      if (GetDXGIFactory(dxgiFactory.GetAddressOf()))
      {
        ComPtr<IDXGIAdapter1> pAdapter1;
        while (dxgiFactory->EnumAdapters1(adapter, pAdapter1.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND)
        {
          DXGI_ADAPTER_DESC1 desc1;
#  if !NDEBUG
          const HRESULT descResult =
#  endif
            pAdapter1->GetDesc1(&desc1);
          assert(SUCCEEDED(descResult));
          constexpr auto basicDriverString = L"Microsoft Basic Render Driver";
          if ((desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && _wcsicmp(basicDriverString, desc1.Description) != 0)
          {
            if (pHardwareAdapter1 == nullptr)
            {
              pHardwareAdapter1 = pAdapter1;
            }
          }
          else if (pFallbackAdapter1 == nullptr)
          {
            pFallbackAdapter1 = pAdapter1;
          }

          ++adapter;
        }
      }
    }

    ComPtr<IDXGIAdapter1> pAdapter1;
    TypeOfDeviceCreated   deviceType = TypeOfDeviceCreated::None;
    if (pHardwareAdapter1 != nullptr)
    {
      pAdapter1  = std::move(pHardwareAdapter1);
      deviceType = TypeOfDeviceCreated::Hardware;
    }
    else if (pFallbackAdapter1 != nullptr)
    {
      pAdapter1  = std::move(pFallbackAdapter1);
      deviceType = TypeOfDeviceCreated::Software;
    }

    if (pAdapter1 == nullptr)
    {
      return TypeOfDeviceCreated::None;
    }

    // Get the IDXGIAdapter interface from the IDXGIAdapter1.
    ComPtr<IDXGIAdapter> pAdapter;
    {
#  if !NDEBUG
      const HRESULT castResult =
#  endif
        pAdapter1->QueryInterface(pAdapter.GetAddressOf());
      assert(SUCCEEDED(castResult));
    }

    static PFN_D3D11_CREATE_DEVICE s_DynamicD3D11CreateDevice = nullptr;

    if (!s_DynamicD3D11CreateDevice)
    {
      HMODULE hModD3D11 = LoadLibraryA("d3d11.dll");
      if (!hModD3D11)
        return TypeOfDeviceCreated::None;

      s_DynamicD3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(reinterpret_cast<void*>(GetProcAddress(hModD3D11, "D3D11CreateDevice")));
      if (!s_DynamicD3D11CreateDevice)
        return TypeOfDeviceCreated::None;
    }

    D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0,
    };

    UINT createDeviceFlags = 0;
    // #  ifdef _DEBUG
    //     don't do this (especially not without a fallback for failure), not needed for texture conversion
    //     createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    // #  endif

    D3D_FEATURE_LEVEL fl;
    HRESULT           hr = s_DynamicD3D11CreateDevice(pAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, createDeviceFlags, featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, &ref_device, &fl, nullptr);

    if (FAILED(hr) && (createDeviceFlags & D3D11_CREATE_DEVICE_DEBUG))
    {
      createDeviceFlags = createDeviceFlags & ~D3D11_CREATE_DEVICE_DEBUG;
      hr                = s_DynamicD3D11CreateDevice(pAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, createDeviceFlags, featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, &ref_device, &fl, nullptr);
    }

    if (SUCCEEDED(hr))
    {
      if (fl < D3D_FEATURE_LEVEL_11_0)
      {
        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
        hr = ref_device->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts));
        if (FAILED(hr))
          memset(&hwopts, 0, sizeof(hwopts));

        if (!hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
        {
          ref_device = nullptr;
          hr         = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
      }
    }

    if (SUCCEEDED(hr))
    {
      DXGI_ADAPTER_DESC desc;
      hr = pAdapter->GetDesc(&desc);
      if (SUCCEEDED(hr))
      {
        xiiStringUtf8 sDesc(desc.Description);
        xiiLog::Dev("Using DirectCompute on \"{0}\"", sDesc.GetData());
      }

      return deviceType;
    }
    else
    {
      return TypeOfDeviceCreated::None;
    }
  }

  /// Ensures a device and its corresponding conversion table are constructed together.
  class DeviceAndConversionTable
  {
  public:
    /// Provides locked access to the table and its DX device.
    class ScopedAccess
    {
    public:
      ScopedAccess(DeviceAndConversionTable& ref_table) :
        m_Lock(ref_table.m_Mutex), m_Table(ref_table)
      {
        ref_table.Init();
      }

      DeviceAndConversionTable* operator->() { return &m_Table; }

    private:
      xiiLock<xiiMutex>         m_Lock;
      DeviceAndConversionTable& m_Table;
    };

    /// Get temporary access to the static DeviceAndConversionTable.
    static ScopedAccess getDeviceAndConversionTable() { return s_DeviceAndConversionTable; }
    static bool         IsDeviceAndConversionTableInitialized() { return s_bDeviceAndTableInitialized; }

    ID3D11Device* getDevice() { return m_D3dDevice.Get(); }

    xiiArrayPtr<const xiiImageConversionEntry> getConvertors() const { return m_SupportedConversions; }

    void Init()
    {
      s_bDeviceAndTableInitialized = true;

      if (!m_SupportedConversions.IsEmpty())
        return;

      // A high penalty for software devices.
      // The number chosen is greater than (32 * 4) * 2 * 2 * 2 = 1024, the estimated cost of the
      // currently largest step, making software DX conversions available but highly undesirable.
      float devicePenalty = 2000.0f;

      if ((CreateDevice(m_D3dDevice) == TypeOfDeviceCreated::Hardware) && !cvar_TexturePenalizeDXConversions)
      {
        // No penalty for hardware devices.
        devicePenalty = 0.0f;
      }

      for (auto& entry : xiiArrayPtr<xiiImageConversionEntry>(s_sourceConversions))
      {
        entry.m_fAdditionalPenalty = devicePenalty;
      }

      m_SupportedConversions = s_sourceConversions;
    }

    void Deinit()
    {
      m_D3dDevice = nullptr;
      m_SupportedConversions.Clear();
      s_bDeviceAndTableInitialized = false;
    }

  private:
    ComPtr<ID3D11Device>                       m_D3dDevice = nullptr;
    xiiArrayPtr<const xiiImageConversionEntry> m_SupportedConversions;

    constexpr static int           s_numConversions = 5;
    static xiiImageConversionEntry s_sourceConversions[s_numConversions];

    xiiMutex                        m_Mutex;
    static bool                     s_bDeviceAndTableInitialized;
    static DeviceAndConversionTable s_DeviceAndConversionTable;
  };

  xiiImageConversionEntry DeviceAndConversionTable::s_sourceConversions[s_numConversions] = {
    xiiImageConversionEntry(xiiGALResourceFormat::RGBA32Float, xiiGALResourceFormat::BC6HUF16, xiiImageConversionFlags::Default),

    xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalized, xiiGALResourceFormat::BC1UNormalized, xiiImageConversionFlags::Default),
    xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalized, xiiGALResourceFormat::BC7UNormalized, xiiImageConversionFlags::Default),

    xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalizedSRGB, xiiGALResourceFormat::BC1UNormalizedSRGB, xiiImageConversionFlags::Default),
    xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalizedSRGB, xiiGALResourceFormat::BC7UNormalizedSRGB, xiiImageConversionFlags::Default),
  };

  bool                     DeviceAndConversionTable::s_bDeviceAndTableInitialized = false;
  DeviceAndConversionTable DeviceAndConversionTable::s_DeviceAndConversionTable;
} // namespace

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(TextureConverter, DXTextureConverterersions)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (DeviceAndConversionTable::IsDeviceAndConversionTableInitialized())
    {
      DeviceAndConversionTable::getDeviceAndConversionTable()->Deinit();
    }
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

class xiiImageConversion_CompressDxTex : public xiiImageConversionStepCompressBlocks
{
public:
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    return DeviceAndConversionTable::getDeviceAndConversionTable()->getConvertors();
  }

  virtual xiiResult CompressBlocks(xiiConstByteBlobPtr pSource, xiiByteBlobPtr pTarget, xiiUInt32 uiNumBlocksX, xiiUInt32 uiNumBlocksY, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    const xiiGALResourceFormatDescription& sourceFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(sourceFormat);
    const xiiGALResourceFormatDescription& targetFormatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(targetFormat);

    const xiiUInt32 uiTargetWidth  = uiNumBlocksX * targetFormatDescription.GetBlockWidth();
    const xiiUInt32 uiTargetHeight = uiNumBlocksY * targetFormatDescription.GetBlockHeight();

    Image sourceImage;
    sourceImage.width      = uiTargetWidth;
    sourceImage.height     = uiTargetHeight;
    sourceImage.rowPitch   = static_cast<size_t>(sourceFormatDescription.GetRowPitch(uiTargetWidth));
    sourceImage.slicePitch = static_cast<size_t>(sourceFormatDescription.GetSlicePitch(uiTargetWidth, uiTargetHeight));

    // We don't trust anyone to handle sRGB correctly, so pretend we always want to compress linear -> linear even when it's actually sRGB -> sRGB.
    sourceImage.format = (DXGI_FORMAT)xiiImageFormatMappings::ToDxgiFormat(xiiGALResourceFormat::AsLinear(sourceFormat));
    sourceImage.pixels = (uint8_t*)static_cast<const void*>(pSource.GetPtr());

    ScratchImage dxSrcImage;
    if (FAILED(dxSrcImage.InitializeFromImage(sourceImage)))
      return XII_FAILURE;

    const DXGI_FORMAT dxgiTargetFormat = (DXGI_FORMAT)xiiImageFormatMappings::ToDxgiFormat(xiiGALResourceFormat::AsLinear(targetFormat));

    ScratchImage dxDstImage;

    bool bCompressionDone = false;

    {
      auto          deviceAndConversionTableScope = DeviceAndConversionTable::getDeviceAndConversionTable();
      ID3D11Device* pD3dDevice                    = deviceAndConversionTableScope->getDevice();
      if (pD3dDevice != nullptr)
      {
        if (SUCCEEDED(Compress(pD3dDevice, dxSrcImage.GetImages(), dxSrcImage.GetImageCount(), dxSrcImage.GetMetadata(), dxgiTargetFormat, TEX_COMPRESS_PARALLEL, 1.0f, dxDstImage)))
        {
          // Not all formats can be compressed on the GPU. Fall back to CPU in case GPU compression fails.
          bCompressionDone = true;
        }
      }
    }

    if (!bCompressionDone)
    {
      if (SUCCEEDED(Compress(dxSrcImage.GetImages(), dxSrcImage.GetImageCount(), dxSrcImage.GetMetadata(), dxgiTargetFormat, TEX_COMPRESS_PARALLEL, 1.0f, dxDstImage)))
      {
        bCompressionDone = true;
      }
    }
    if (!bCompressionDone)
    {
      if (SUCCEEDED(Compress(dxSrcImage.GetImages(), dxSrcImage.GetImageCount(), dxSrcImage.GetMetadata(), dxgiTargetFormat, TEX_COMPRESS_DEFAULT, 1.0f, dxDstImage)))
      {
        bCompressionDone = true;
      }
    }

    if (!bCompressionDone)
      return XII_FAILURE;

    pTarget.CopyFrom(xiiConstByteBlobPtr(dxDstImage.GetPixels(), static_cast<xiiUInt32>(dxDstImage.GetPixelsSize())));

    return XII_SUCCESS;
  }
};

XII_STATICLINK_FORCE static xiiImageConversion_CompressDxTex s_conversion_compressDxTex;

#endif

XII_STATICLINK_FILE(Texture, Texture_Image_Conversions_DXTextureConverterersions);

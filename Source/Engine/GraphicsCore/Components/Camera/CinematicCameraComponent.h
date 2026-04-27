#pragma once

#include <Core/World/World.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// \brief Film format presets for the cinematic camera.
struct XII_GRAPHICSCORE_DLL xiiFilmFormat
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    FullFrame35mm = 0, ///< 36 x 24 mm sensor
    Super35,           ///< 24.89 x 18.67 mm sensor
    AnamorphicScope,   ///< 2.39:1 anamorphic
    IMAX70mm,          ///< 70 mm IMAX
    Custom,

    ENUM_COUNT,
    Default = FullFrame35mm
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiFilmFormat);

/// \brief Render data for the cinematic camera.
class XII_GRAPHICSCORE_DLL xiiCinematicCameraRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCinematicCameraRenderData, xiiRenderData);

public:
  float                  m_fFocalLength  = 50.0f; ///< mm
  float                  m_fAperture     = 2.8f;  ///< f-stop
  float                  m_fFocusDist    = 5.0f;  ///< metres
  float                  m_fSensorWidth  = 36.0f; ///< mm
  float                  m_fSensorHeight = 24.0f; ///< mm
  xiiEnum<xiiFilmFormat> m_FilmFormat;
  bool                   m_bEnableDoF = false;
};

using xiiCinematicCameraComponentManager = xiiComponentManager<class xiiCinematicCameraComponent, xiiBlockStorageType::Compact>;

/// \brief Extends the base camera with physically-based cinematic lens controls.
class XII_GRAPHICSCORE_DLL xiiCinematicCameraComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiCinematicCameraComponent, xiiComponent, xiiCinematicCameraComponentManager);

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  xiiCinematicCameraComponent();
  ~xiiCinematicCameraComponent();

  void  SetFocalLength(float fMM);                        // [ property ]
  float GetFocalLength() const { return m_fFocalLength; } // [ property ]

  void  SetAperture(float fStop);                   // [ property ]
  float GetAperture() const { return m_fAperture; } // [ property ]

  void  SetFocusDist(float fMetres);                  // [ property ]
  float GetFocusDist() const { return m_fFocusDist; } // [ property ]

  void                   SetFilmFormat(xiiEnum<xiiFilmFormat> fmt);     // [ property ]
  xiiEnum<xiiFilmFormat> GetFilmFormat() const { return m_FilmFormat; } // [ property ]

  void SetEnableDoF(bool b);                         // [ property ]
  bool GetEnableDoF() const { return m_bEnableDoF; } // [ property ]

  /// \brief Computes horizontal field-of-view from focal length and sensor width.
  xiiAngle ComputeHFOV() const;

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  float                  m_fFocalLength  = 50.0f;
  float                  m_fAperture     = 2.8f;
  float                  m_fFocusDist    = 5.0f;
  float                  m_fSensorWidth  = 36.0f;
  float                  m_fSensorHeight = 24.0f;
  xiiEnum<xiiFilmFormat> m_FilmFormat    = xiiFilmFormat::FullFrame35mm;
  bool                   m_bEnableDoF    = false;
};

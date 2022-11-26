#if 0

#  pragma once

#  include <JoltPlugin/Constraints/JoltConstraintComponent.h>

struct XII_JOLTPLUGIN_DLL xiiJoltAxis
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    None = 0,
    X = XII_BIT(0),
    Y = XII_BIT(1),
    Z = XII_BIT(2),
    All = X | Y | Z,
    Default = All
  };

  struct Bits
  {
    StorageType X : 1;
    StorageType Y : 1;
    StorageType Z : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiJoltAxis);
XII_DECLARE_REFLECTABLE_TYPE(XII_JOLTPLUGIN_DLL, xiiJoltAxis);

using xiiJolt6DOFConstraintComponentManager = xiiComponentManager<class xiiJolt6DOFConstraintComponent, xiiBlockStorageType::Compact>;

class XII_JOLTPLUGIN_DLL xiiJolt6DOFConstraintComponent : public xiiJoltConstraintComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJolt6DOFConstraintComponent, xiiJoltConstraintComponent, xiiJolt6DOFConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJoltConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;


  //////////////////////////////////////////////////////////////////////////
  // xiiJolt6DOFConstraintComponent

public:
  xiiJolt6DOFConstraintComponent();
  ~xiiJolt6DOFConstraintComponent();

  virtual void ApplySettings() final override;

  void SetFreeLinearAxis(xiiBitflags<xiiJoltAxis> flags);                         // [ property ]
  xiiBitflags<xiiJoltAxis> GetFreeLinearAxis() const { return m_FreeLinearAxis; } // [ property ]

  void SetFreeAngularAxis(xiiBitflags<xiiJoltAxis> flags);                          // [ property ]
  xiiBitflags<xiiJoltAxis> GetFreeAngularAxis() const { return m_FreeAngularAxis; } // [ property ]

  void SetLinearLimitMode(xiiJoltConstraintLimitMode::Enum mode);                           // [ property ]
  xiiJoltConstraintLimitMode::Enum GetLinearLimitMode() const { return m_LinearLimitMode; } // [ property ]

  void SetLinearRangeX(const xiiVec2& value);                        // [ property ]
  const xiiVec2& GetLinearRangeX() const { return m_vLinearRangeX; } // [ property ]
  void SetLinearRangeY(const xiiVec2& value);                        // [ property ]
  const xiiVec2& GetLinearRangeY() const { return m_vLinearRangeY; } // [ property ]
  void SetLinearRangeZ(const xiiVec2& value);                        // [ property ]
  const xiiVec2& GetLinearRangeZ() const { return m_vLinearRangeZ; } // [ property ]

  void SetLinearStiffness(float f);                               // [ property ]
  float GetLinearStiffness() const { return m_fLinearStiffness; } // [ property ]

  void SetLinearDamping(float f);                             // [ property ]
  float GetLinearDamping() const { return m_fLinearDamping; } // [ property ]

  void SetSwingLimitMode(xiiJoltConstraintLimitMode::Enum mode);                          // [ property ]
  xiiJoltConstraintLimitMode::Enum GetSwingLimitMode() const { return m_SwingLimitMode; } // [ property ]

  void SetSwingLimit(xiiAngle f);                         // [ property ]
  xiiAngle GetSwingLimit() const { return m_SwingLimit; } // [ property ]

  void SetSwingStiffness(float f);                              // [ property ]
  float GetSwingStiffness() const { return m_fSwingStiffness; } // [ property ]

  void SetSwingDamping(float f);                            // [ property ]
  float GetSwingDamping() const { return m_fSwingDamping; } // [ property ]

  void SetTwistLimitMode(xiiJoltConstraintLimitMode::Enum mode);                          // [ property ]
  xiiJoltConstraintLimitMode::Enum GetTwistLimitMode() const { return m_TwistLimitMode; } // [ property ]

  void SetLowerTwistLimit(xiiAngle f);                              // [ property ]
  xiiAngle GetLowerTwistLimit() const { return m_LowerTwistLimit; } // [ property ]

  void SetUpperTwistLimit(xiiAngle f);                              // [ property ]
  xiiAngle GetUpperTwistLimit() const { return m_UpperTwistLimit; } // [ property ]

  void SetTwistStiffness(float f);                              // [ property ]
  float GetTwistStiffness() const { return m_fTwistStiffness; } // [ property ]

  void SetTwistDamping(float f);                            // [ property ]
  float GetTwistDamping() const { return m_fTwistDamping; } // [ property ]

protected:
  xiiBitflags<xiiJoltAxis> m_FreeLinearAxis;

  xiiEnum<xiiJoltConstraintLimitMode> m_LinearLimitMode;

  float m_fLinearStiffness = 0.0f;
  float m_fLinearDamping = 0.0f;

  xiiVec2 m_vLinearRangeX = xiiVec2::ZeroVector();
  xiiVec2 m_vLinearRangeY = xiiVec2::ZeroVector();
  xiiVec2 m_vLinearRangeZ = xiiVec2::ZeroVector();

  xiiBitflags<xiiJoltAxis> m_FreeAngularAxis;

  xiiEnum<xiiJoltConstraintLimitMode> m_SwingLimitMode;
  xiiAngle m_SwingLimit;

  float m_fSwingStiffness = 0.0f; // [ property ]
  float m_fSwingDamping = 0.0f;   // [ property ]

  xiiEnum<xiiJoltConstraintLimitMode> m_TwistLimitMode;
  xiiAngle m_LowerTwistLimit;
  xiiAngle m_UpperTwistLimit;

  float m_fTwistStiffness = 0.0f; // [ property ]
  float m_fTwistDamping = 0.0f;   // [ property ]
};

#endif

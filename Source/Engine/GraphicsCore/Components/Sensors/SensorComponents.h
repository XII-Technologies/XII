#pragma once
#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>
struct xiiMsgExtractRenderData;

#define XII_SENSOR_COMP(Name, RDName, Manager, ...)                                                            \
  class XII_GRAPHICSCORE_DLL RDName : public xiiRenderData                                                     \
  {                                                                                                            \
    XII_ADD_DYNAMIC_REFLECTION(RDName, xiiRenderData);                                                         \
                                                                                                               \
  public:                                                                                                      \
    __VA_ARGS__                                                                                                \
  };                                                                                                           \
  using Manager = xiiComponentManager<class Name, xiiBlockStorageType::Compact>;                               \
  class XII_GRAPHICSCORE_DLL Name : public xiiRenderComponent                                                  \
  {                                                                                                            \
    XII_DECLARE_COMPONENT_TYPE(Name, xiiRenderComponent, Manager);                                             \
                                                                                                               \
  public:                                                                                                      \
    virtual void      SerializeComponent(xiiWorldWriter& s) const override;                                    \
    virtual void      DeserializeComponent(xiiWorldReader& s) override;                                        \
    virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) override; \
    Name();                                                                                                    \
    ~Name();                                                                                                   \
                                                                                                               \
  protected:                                                                                                   \
    void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

XII_SENSOR_COMP(xiiPhysicsDebugRenderComponent, xiiPhysicsDebugRenderData, xiiPhysicsDebugRenderComponentManager,
                bool m_bShowColliders = true;
                bool m_bShowJoints = true; bool m_bShowContacts = false;)
bool m_bShowColliders = true;
bool m_bShowJoints    = true;
bool m_bShowContacts  = false;
void SetShowColliders(bool b);
bool GetShowColliders() const { return m_bShowColliders; }
void SetShowJoints(bool b);
bool GetShowJoints() const { return m_bShowJoints; }
void SetShowContacts(bool b);
bool GetShowContacts() const { return m_bShowContacts; }
}
;

XII_SENSOR_COMP(xiiFieldVisualizerComponent, xiiFieldVisualizerRenderData, xiiFieldVisualizerComponentManager,
                xiiUInt8 m_uiFieldType = 0;
                float    m_fScale      = 1.0f;)
xiiUInt8 m_uiFieldType = 0;
float    m_fScale      = 1.0f;
}
;

XII_SENSOR_COMP(xiiMolecularVisualizerComponent, xiiMolecularVisualizerRenderData, xiiMolecularVisualizerComponentManager,
                float m_fAtomRadius = 0.1f;
                bool  m_bShowBonds  = true;)
float m_fAtomRadius = 0.1f;
bool  m_bShowBonds  = true;
}
;

XII_SENSOR_COMP(xiiParticleSimulationComponent, xiiParticleSimulationRenderData, xiiParticleSimulationComponentManager,
                xiiUInt32 m_uiParticleCount = 10000;
                float     m_fTimeStep       = 0.016f;)
xiiUInt32 m_uiParticleCount = 10000;
float     m_fTimeStep       = 0.016f;
}
;

XII_SENSOR_COMP(xiiLiDARSensorComponent, xiiLiDARSensorRenderData, xiiLiDARSensorComponentManager,
                xiiUInt32 m_uiRayCount = 64;
                float m_fRange = 100.0f; float m_fHFOV = 360.0f;)
xiiUInt32 m_uiRayCount = 64;
float     m_fRange     = 100.0f;
float     m_fHFOV      = 360.0f;
void      SetRayCount(xiiUInt32 n);
xiiUInt32 GetRayCount() const { return m_uiRayCount; }
void      SetRange(float f);
float     GetRange() const { return m_fRange; }
}
;

XII_SENSOR_COMP(xiiDepthSensorComponent, xiiDepthSensorRenderData, xiiDepthSensorComponentManager,
                xiiUInt32 m_uiWidth  = 640;
                xiiUInt32 m_uiHeight = 480; float m_fMinDepth = 0.1f; float m_fMaxDepth = 10.0f;)
xiiUInt32 m_uiWidth   = 640;
xiiUInt32 m_uiHeight  = 480;
float     m_fMinDepth = 0.1f;
float     m_fMaxDepth = 10.0f;
}
;

XII_SENSOR_COMP(xiiThermalSensorComponent, xiiThermalSensorRenderData, xiiThermalSensorComponentManager,
                float m_fMinTemp = -20.0f;
                float m_fMaxTemp = 100.0f; xiiUInt32 m_uiWidth = 320; xiiUInt32 m_uiHeight = 240;)
float     m_fMinTemp = -20.0f;
float     m_fMaxTemp = 100.0f;
xiiUInt32 m_uiWidth  = 320;
xiiUInt32 m_uiHeight = 240;
}
;

XII_SENSOR_COMP(xiiSensorFusionComponent, xiiSensorFusionRenderData, xiiSensorFusionComponentManager,
                xiiUInt8 m_uiActiveSensorMask = 0xFF;)
xiiUInt8 m_uiActiveSensorMask = 0xFF;
void     SetSensorMask(xiiUInt8 mask);
xiiUInt8 GetSensorMask() const { return m_uiActiveSensorMask; }
}
;

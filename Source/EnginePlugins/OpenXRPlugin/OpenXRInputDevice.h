#pragma once

#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>

class xiiOpenXR;

XII_DEFINE_AS_POD_TYPE(XrActionSuggestedBinding);
XII_DEFINE_AS_POD_TYPE(XrActiveActionSet);

class XII_OPENXRPLUGIN_DLL xiiOpenXRInputDevice : public xiiXRInputDevice
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOpenXRInputDevice, xiiXRInputDevice);

public:
  void                             GetDeviceList(xiiHybridArray<xiiXRDeviceID, 64>& out_Devices) const override;
  xiiXRDeviceID                    GetDeviceIDByType(xiiXRDeviceType::Enum type) const override;
  const xiiXRDeviceState&          GetDeviceState(xiiXRDeviceID iDeviceID) const override;
  xiiString                        GetDeviceName(xiiXRDeviceID iDeviceID) const override;
  xiiBitflags<xiiXRDeviceFeatures> GetDeviceFeatures(xiiXRDeviceID iDeviceID) const override;

private:
  friend class xiiOpenXR;
  struct Bind
  {
    XrAction    action;
    const char* szPath;
  };

  struct Action
  {
    xiiXRDeviceFeatures::Enum m_Feature;
    XrAction                  m_Action;
    xiiString                 m_sKey[2];
  };

  struct Vec2Action
  {
    Vec2Action(xiiXRDeviceFeatures::Enum feature, XrAction action, xiiStringView sLeft, xiiStringView sRight);
    xiiXRDeviceFeatures::Enum m_Feature;
    XrAction                  m_Action;
    xiiString                 m_sKey_negx[2];
    xiiString                 m_sKey_posx[2];
    xiiString                 m_sKey_negy[2];
    xiiString                 m_sKey_posy[2];
  };

  xiiOpenXRInputDevice(xiiOpenXR* pOpenXR);
  XrResult CreateActions(XrSession session, XrSpace m_sceneSpace);
  void     DestroyActions();

  XrPath   CreatePath(const char* szPath);
  XrResult CreateAction(xiiXRDeviceFeatures::Enum feature, const char* actionName, XrActionType actionType, XrAction& out_action);
  XrResult SuggestInteractionProfileBindings(const char* szInteractionProfile, const char* szNiceName, xiiArrayPtr<Bind> bindings);
  XrResult AttachSessionActionSets(XrSession session);
  XrResult UpdateCurrentInteractionProfile();

  void InitializeDevice() override;
  void RegisterInputSlots() override;
  void UpdateInputSlotValues() override {}

  XrResult UpdateActions();
  void     UpdateControllerState();

private:
  xiiOpenXR* m_pOpenXR  = nullptr;
  XrInstance m_instance = XR_NULL_HANDLE;
  XrSession  m_session  = XR_NULL_HANDLE;

  xiiXRDeviceState                 m_DeviceState[3]; // Hard-coded for now
  xiiString                        m_sActiveProfile[3];
  xiiBitflags<xiiXRDeviceFeatures> m_SupportedFeatures[3];
  const xiiInt8                    m_iLeftControllerDeviceID  = 1;
  const xiiInt8                    m_iRightControllerDeviceID = 2;

  XrActionSet                     m_ActionSet = XR_NULL_HANDLE;
  xiiHashTable<XrPath, xiiString> m_InteractionProfileToNiceName;

  xiiStaticArray<const char*, 2> m_subActionPrefix;
  xiiStaticArray<XrPath, 2>      m_subActionPath;

  xiiHybridArray<Action, 4>     m_booleanActions;
  xiiHybridArray<Action, 4>     m_floatActions;
  xiiHybridArray<Vec2Action, 4> m_vec2Actions;
  xiiHybridArray<Action, 4>     m_poseActions;

  XrSpace m_gripSpace[2];
  XrSpace m_aimSpace[2];
};

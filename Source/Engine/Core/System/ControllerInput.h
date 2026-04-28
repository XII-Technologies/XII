/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>

class xiiInputDeviceController;

class XII_CORE_DLL xiiControllerInput
{
public:
  // \brief Returns if a global controller input device exists.
  static bool HasDevice();

  // \brief Returns the global controller input device. May be nullptr.
  static xiiInputDeviceController* GetDevice();

  // \brief Set the global controller input device.
  static void SetDevice(xiiInputDeviceController* pDevice);
};

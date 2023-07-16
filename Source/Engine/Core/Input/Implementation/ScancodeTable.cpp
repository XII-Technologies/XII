#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

xiiStringView xiiInputManager::ConvertScanCodeToEngineName(xiiUInt8 uiScanCode, bool bIsExtendedKey)
{
  const xiiUInt8 uiFinalScanCode = bIsExtendedKey ? (uiScanCode + 128) : uiScanCode;

  switch (uiFinalScanCode)
  {
    case 1:
      return xiiInputSlot_KeyEscape;
    case 2:
      return xiiInputSlot_Key1;
    case 3:
      return xiiInputSlot_Key2;
    case 4:
      return xiiInputSlot_Key3;
    case 5:
      return xiiInputSlot_Key4;
    case 6:
      return xiiInputSlot_Key5;
    case 7:
      return xiiInputSlot_Key6;
    case 8:
      return xiiInputSlot_Key7;
    case 9:
      return xiiInputSlot_Key8;
    case 10:
      return xiiInputSlot_Key9;
    case 11:
      return xiiInputSlot_Key0;
    case 12:
      return xiiInputSlot_KeyHyphen;
    case 13:
      return xiiInputSlot_KeyEquals;
    case 14:
      return xiiInputSlot_KeyBackspace;
    case 15:
      return xiiInputSlot_KeyTab;
    case 16:
      return xiiInputSlot_KeyQ;
    case 17:
      return xiiInputSlot_KeyW;
    case 18:
      return xiiInputSlot_KeyE;
    case 19:
      return xiiInputSlot_KeyR;
    case 20:
      return xiiInputSlot_KeyT;
    case 21:
      return xiiInputSlot_KeyY;
    case 22:
      return xiiInputSlot_KeyU;
    case 23:
      return xiiInputSlot_KeyI;
    case 24:
      return xiiInputSlot_KeyO;
    case 25:
      return xiiInputSlot_KeyP;
    case 26:
      return xiiInputSlot_KeyBracketOpen;
    case 27:
      return xiiInputSlot_KeyBracketClose;
    case 28:
      return xiiInputSlot_KeyReturn;
    case 29:
      return xiiInputSlot_KeyLeftCtrl;
    case 30:
      return xiiInputSlot_KeyA;
    case 31:
      return xiiInputSlot_KeyS;
    case 32:
      return xiiInputSlot_KeyD;
    case 33:
      return xiiInputSlot_KeyF;
    case 34:
      return xiiInputSlot_KeyG;
    case 35:
      return xiiInputSlot_KeyH;
    case 36:
      return xiiInputSlot_KeyJ;
    case 37:
      return xiiInputSlot_KeyK;
    case 38:
      return xiiInputSlot_KeyL;
    case 39:
      return xiiInputSlot_KeySemicolon;
    case 40:
      return xiiInputSlot_KeyApostrophe;
    case 41:
      return xiiInputSlot_KeyTilde;
    case 42:
      return xiiInputSlot_KeyLeftShift;
    case 43:
      return xiiInputSlot_KeyBackslash;
    case 44:
      return xiiInputSlot_KeyZ;
    case 45:
      return xiiInputSlot_KeyX;
    case 46:
      return xiiInputSlot_KeyC;
    case 47:
      return xiiInputSlot_KeyV;
    case 48:
      return xiiInputSlot_KeyB;
    case 49:
      return xiiInputSlot_KeyN;
    case 50:
      return xiiInputSlot_KeyM;
    case 51:
      return xiiInputSlot_KeyComma;
    case 52:
      return xiiInputSlot_KeyPeriod;
    case 53:
      return xiiInputSlot_KeySlash;
    case 54:
      return xiiInputSlot_KeyRightShift;
    case 55:
      return xiiInputSlot_KeyNumpadStar;
    case 56:
      return xiiInputSlot_KeyLeftAlt;
    case 57:
      return xiiInputSlot_KeySpace;
    case 58:
      return xiiInputSlot_KeyCapsLock;
    case 59:
      return xiiInputSlot_KeyF1;
    case 60:
      return xiiInputSlot_KeyF2;
    case 61:
      return xiiInputSlot_KeyF3;
    case 62:
      return xiiInputSlot_KeyF4;
    case 63:
      return xiiInputSlot_KeyF5;
    case 64:
      return xiiInputSlot_KeyF6;
    case 65:
      return xiiInputSlot_KeyF7;
    case 66:
      return xiiInputSlot_KeyF8;
    case 67:
      return xiiInputSlot_KeyF9;
    case 68:
      return xiiInputSlot_KeyF10;
    case 69:
      return xiiInputSlot_KeyNumLock;
    case 70:
      return xiiInputSlot_KeyScroll;
    case 71:
      return xiiInputSlot_KeyNumpad7;
    case 72:
      return xiiInputSlot_KeyNumpad8;
    case 73:
      return xiiInputSlot_KeyNumpad9;
    case 74:
      return xiiInputSlot_KeyNumpadMinus;
    case 75:
      return xiiInputSlot_KeyNumpad4;
    case 76:
      return xiiInputSlot_KeyNumpad5;
    case 77:
      return xiiInputSlot_KeyNumpad6;
    case 78:
      return xiiInputSlot_KeyNumpadPlus;
    case 79:
      return xiiInputSlot_KeyNumpad1;
    case 80:
      return xiiInputSlot_KeyNumpad2;
    case 81:
      return xiiInputSlot_KeyNumpad3;
    case 82:
      return xiiInputSlot_KeyNumpad0;
    case 83:
      return xiiInputSlot_KeyNumpadPeriod;


    case 86:
      return xiiInputSlot_KeyPipe;
    case 87:
      return xiiInputSlot_KeyF11;
    case 88:
      return xiiInputSlot_KeyF12;


    case 91:
      return xiiInputSlot_KeyLeftWin;
    case 92:
      return xiiInputSlot_KeyRightWin;
    case 93:
      return xiiInputSlot_KeyApps;



    case 128 + 16:
      return xiiInputSlot_KeyPrevTrack;
    case 128 + 25:
      return xiiInputSlot_KeyNextTrack;
    case 128 + 28:
      return xiiInputSlot_KeyNumpadEnter;
    case 128 + 29:
      return xiiInputSlot_KeyRightCtrl;
    case 128 + 32:
      return xiiInputSlot_KeyMute;
    case 128 + 34:
      return xiiInputSlot_KeyPlayPause;
    case 128 + 36:
      return xiiInputSlot_KeyStop;
    case 128 + 46:
      return xiiInputSlot_KeyVolumeDown;
    case 128 + 48:
      return xiiInputSlot_KeyVolumeUp;
    case 128 + 53:
      return xiiInputSlot_KeyNumpadSlash;
    case 128 + 55:
      return xiiInputSlot_KeyPrint;
    case 128 + 56:
      return xiiInputSlot_KeyRightAlt;
    case 128 + 70:
      return xiiInputSlot_KeyPause;
    case 128 + 71:
      return xiiInputSlot_KeyHome;
    case 128 + 72:
      return xiiInputSlot_KeyUp;
    case 128 + 73:
      return xiiInputSlot_KeyPageUp;
    case 128 + 75:
      return xiiInputSlot_KeyLeft;
    case 128 + 77:
      return xiiInputSlot_KeyRight;
    case 128 + 79:
      return xiiInputSlot_KeyEnd;
    case 128 + 80:
      return xiiInputSlot_KeyDown;
    case 128 + 81:
      return xiiInputSlot_KeyPageDown;
    case 128 + 82:
      return xiiInputSlot_KeyInsert;
    case 128 + 83:
      return xiiInputSlot_KeyDelete;

    default:

      // For extended keys fall back to the non-extended name
      if (bIsExtendedKey)
        return ConvertScanCodeToEngineName(uiScanCode, false);

      break;
  }

  return "unknown_key";
}

XII_STATICLINK_FILE(Core, Core_Input_Implementation_ScancodeTable);

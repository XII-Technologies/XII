/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>

class XII_FOUNDATION_DLL xiiDataTransfer;

/// A small wrapper class around a xiiTelemetryMessage for sending a 'data transfer'. See xiiDataTransfer for more details.
class XII_FOUNDATION_DLL xiiDataTransferObject
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiDataTransferObject);

public:
  /// xiiDataTransferObject instances should always be created on the stack and should be very short lived.
  ///
  /// \param BelongsTo
  ///   The xiiDataTransfer through which the data is going to be sent shortly.
  /// \param szObjectName
  ///   The name of the data blob. Since several data transfers can be made through the same xiiDataTransfer, this is used to identify
  ///   the different pieces of information. For example when sending each texture of a G-Buffer, szObjectName could be 'diffuse' when
  ///   sending the diffuse channel, 'depth' for the depth buffer content, etc.
  /// \param szMimeType
  ///   The mime type for the data blob. Tools such as xiiInspector can use this information to determine whether they can display it
  ///   directly or in some other editor.
  /// \param szFileExtension
  ///   The file extension that should be used when the user wants to store the data on disk. This is separate from the mime type,
  ///   as you might want to send data with mime type 'text/json' but store it on disk as '.savegame'.
  xiiDataTransferObject(xiiDataTransfer& ref_belongsTo, xiiStringView sObjectName, xiiStringView sMimeType, xiiStringView sFileExtension);

  /// The destructor will assert if the data has not been transmitted.
  ~xiiDataTransferObject();

  /// Returns the stream writer that you need to use to write the data into the object.
  ///
  /// When finished writing all data to the object, you should call Transmit().
  xiiStreamWriter& GetWriter() { return m_Msg.GetWriter(); }

  /// Will initiate the data transfer.
  void Transmit();

private:
  friend class xiiDataTransfer;

  bool                m_bHasBeenTransferred;
  xiiDataTransfer&    m_BelongsTo;
  xiiTelemetryMessage m_Msg;
};

/// A 'data transfer' is a blob of data that an application can send to connected tools such as xiiInspector upon request.
///
/// Data transfers can be used to allow an application to send large amounts of data to tools such as xiiInspector, which can then
/// display or process them somehow. An example usage is to send a screenshot or the different textures of the G-Buffer for analysis.
/// Or an application could send the current game state as one large text or binary document.
/// The data transfer can contain any kind of data, however depending on its type, the connected tools may be able to display the data
/// directly, or not.
/// The xiiDataTransfer object represents one logical group of data that will be sent all together. Each piece of data is represented
/// by a xiiDataTransferObject. E.g. each texture of a G-Buffer (diffuse, normals, depth) should be sent via one instance of xiiDataTransferObject.
/// Those instances of xiiDataTransferObject are just created on demand and destroyed directly after, they do not need to be kept around.
///
/// The xiiDataTransfer instance, however, needs to be created once and then kept around as long as the data transfer should be possible.
/// It is additionally necessary to enable the data transfer by calling EnableDataTransfer() once, which also specifies under which name
/// the transfer appears in the connected tools.
///
/// At runtime the application needs to check every xiiDataTransfer object regularly whether IsTransferRequested() returns true. If so,
/// the application should prepare all data by putting it into instances of xiiDataTransferObject and then transferring them through the
/// xiiDataTransfer object via xiiDataTransfer::Transfer().
///
/// The tools can request a data transfer at any time, however they will not block for the result. Thus whether an application 'answers'
/// or not, is not a problem, the application may just ignore the request. Similarly, the application may also 'push' out a data transfer,
/// even when no data was requested. However, this can overload the network bandwidth or the computer that runs xiiInspector, if not done
/// carefully.
class XII_FOUNDATION_DLL xiiDataTransfer
{
public:
  /// By default the data transfer is deactivated.
  xiiDataTransfer();

  /// Deactivates the data transfer (sends this info via xiiTelemetry).
  virtual ~xiiDataTransfer();

  /// Disables the data transfer. It will not show up in xiiInspector anymore and calling Transfer() on it will be ignored.
  void DisableDataTransfer();

  /// Enables the data transfer. It will show up with the given name in xiiInspector.
  void EnableDataTransfer(xiiStringView sDataName);

  /// Sets the IsTransferRequested() state to true. Ignored if the data transfer is disabled.
  void RequestDataTransfer();

  /// Returns whether the data transfer has been requested. Always returns false while the data transfer is disabled.
  ///
  /// If bReset is set to false, the request state will not be reset, which can be used to 'peek' at the current state.
  /// By default the request state is reset to false afterwards, the application should answer the request.
  bool IsTransferRequested(bool bReset = true);

private:
  virtual void OnTransferRequest() {}

  void SendStatus();

  /// The data in the given xiiDataTransferObject is sent via xiiTelemetry to all connected tools, which can then display or process it.
  void Transfer(xiiDataTransferObject& Object);

private:
  friend class xiiDataTransferObject;

  static void TelemetryMessage(void* pPassThrough);
  static void TelemetryEventsHandler(const xiiTelemetry::TelemetryEventData& e);
  static void Initialize();
  static void SendAllDataTransfers();

  static bool s_bInitialized;

  bool                            m_bEnabled;
  bool                            m_bTransferRequested;
  xiiString                       m_sDataName;
  static xiiSet<xiiDataTransfer*> s_AllTransfers;
};

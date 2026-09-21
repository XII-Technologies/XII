/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

class xiiRemoteInterface;

/// Interface to give access to the FileServe client for additional tooling needs.
///
/// For now, this interface just gives access to the xiiRemoteInterface that is used to communicate with the FileServe server.
/// This allows for maximum flexibility sending and receiving custom messages.
class xiiRemoteToolingInterface
{
public:
  virtual xiiRemoteInterface* GetRemoteInterface() = 0;
};

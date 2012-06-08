/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/ModuleUtils.h"

#include "CameraSupport.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(CameraSupport)
NS_DEFINE_NAMED_CID(CameraSupport_CID);

static const mozilla::Module::CIDEntry kB2GCIDs[] = {
  { &kCameraSupport_CID, false, NULL, CameraSupportConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kB2GContracts[] = {
  { CameraSupport_ContractID, &kCameraSupport_CID },
  { NULL }
};

static const mozilla::Module kB2GModule = {
  mozilla::Module::kVersion,
  kB2GCIDs,
  kB2GContracts
};

NSMODULE_DEFN(b2gmodule) = &kB2GModule;

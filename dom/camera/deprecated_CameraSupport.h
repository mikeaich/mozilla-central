/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef B2G_CAMERASUPPORT_H
#define B2G_CAMERASUPPORT_H

#include "nsIDOMCameraManager.h"
#include "nsIClassInfo.h"

class CameraSupport : public nsICameraSupport
                    , public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICAMERASUPPORT
  NS_DECL_NSICLASSINFO

  CameraSupport();

private:
  ~CameraSupport();

};

#define CameraSupport_CID \
{0x1b5bbf78, 0xa5e1, 0x4a75, {0x99, 0x3e, 0x8a, 0x7e, 0xfc, 0xa7, 0x76, 0x6f}}

#define CameraSupport_ContractID "@mozilla.org/b2g/camera-support;1"

#endif

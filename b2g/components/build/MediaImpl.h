/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef B2G_MEDIAIMPL_H
#define B2G_MEDIAIMPL_H

#include "nsComponentManagerUtils.h"
#include "nsIClassInfo.h"
#include "nsIProgrammingLanguage.h"
#include "nsIClassInfoImpl.h"

class MediaImpl {
public:
  static PRUint32 getNumberOfCameras();
  static NS_IMETHODIMP GetCameraStream(const JS::Value & aOptions, JSContext* cx, nsIDOMMediaStream * *_retval NS_OUTPARAM);
};

#endif

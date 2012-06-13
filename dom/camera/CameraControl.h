/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_NSCAMERACONTROL_H
#define DOM_CAMERA_NSCAMERACONTROL_H


#include "prtypes.h"
#include "nsIDOMCameraManager.h"


class nsCameraControl : public nsICameraControl
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICAMERACONTROL

  nsCameraControl();

  const char* GetParameter(const char* key);
  void SetParameter(const char* key, const char* value);

  void ReceiveImage(PRUint8* aData, PRUint32 aLength);
  void AutoFocusComplete(bool success);
  void ReceiveFrame(PRUint8* aData, PRUint32 aLength);

private:
  ~nsCameraControl();

protected:
  /* additional members */
  PRUint32  mHwHandle;
};


#endif // DOM_CAMERA_NSCAMERACONTROL_H

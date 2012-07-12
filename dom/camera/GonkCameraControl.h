/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_GONKCAMERACONTROL_H
#define DOM_CAMERA_GONKCAMERACONTROL_H


#include "prtypes.h"
#include "CameraCommon.h"


BEGIN_CAMERA_NAMESPACE

class nsCameraControl;

/* camera driver callbacks */
void GonkCameraReceiveImage(nsCameraControl* gc, PRUint8* aData, PRUint32 aLength);
void GonkCameraAutoFocusComplete(nsCameraControl* gc, bool success);
void GonkCameraReceiveFrame(nsCameraControl* gc, PRUint8* aData, PRUint32 aLength);

END_CAMERA_NAMESPACE


#endif // DOM_CAMERA_GONKCAMERACONTROL_H

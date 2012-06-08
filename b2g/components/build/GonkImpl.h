/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef GONK_IMPL_H
#define GONK_IMPL_H


class GonkCamera;

/* camera driver callbacks */
void GonkCameraReceiveImage(GonkCamera* gc, PRUint8* aData, PRUint32 aLength);
void GonkCameraAutoFocusComplete(GonkCamera* gc, bool success);
void GonkCameraReceiveFrame(GonkCamera* gc, PRUint8* aData, PRUint32 aLength);
void GonkCameraAddRef(GonkCamera* gc);
void GonkCameraRelease(GonkCamera* gc);


#endif // GONK_IMPL_H

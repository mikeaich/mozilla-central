/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
 
#ifndef DOM_CAMERA_GONKCAMERAPREVIEW_H
#define DOM_CAMERA_GONKCAMERAPREVIEW_H


#include "CameraPreview.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"


BEGIN_CAMERA_NAMESPACE

class GonkCameraPreview : public CameraPreview
{
public:
  GonkCameraPreview(PRUint32 aHwHandle, PRUint32 aWidth, PRUint32 aHeight)
    : CameraPreview(aWidth, aHeight)
    , mHwHandle(aHwHandle)
    , mDiscardedFrameCount(0)
  { }
  ~GonkCameraPreview()
  {
    Stop();
  }

  void ReceiveFrame(PRUint8 *aData, PRUint32 aLength);

  void Start();
  void Stop();

protected:
  PRUint32 mHwHandle;
  PRUint32 mDiscardedFrameCount;

private:
  GonkCameraPreview(const GonkCameraPreview&);
  GonkCameraPreview& operator=(const GonkCameraPreview&);
};

END_CAMERA_NAMESPACE


#endif // DOM_CAMERA_GONKCAMERAPREVIEW_H
 
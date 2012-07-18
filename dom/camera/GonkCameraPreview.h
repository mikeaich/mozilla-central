/*
 * Copyright (C) 2012 Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
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
    , mFormat(GonkCameraHardware::PREVIEW_FORMAT_UNKNOWN)
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
  PRUint32 mFormat;

private:
  GonkCameraPreview(const GonkCameraPreview&);
  GonkCameraPreview& operator=(const GonkCameraPreview&);
};

END_CAMERA_NAMESPACE

#endif // DOM_CAMERA_GONKCAMERAPREVIEW_H
 
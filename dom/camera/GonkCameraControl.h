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

#ifndef DOM_CAMERA_GONKCAMERACONTROL_H
#define DOM_CAMERA_GONKCAMERACONTROL_H

#include "prtypes.h"
#include "prrwlock.h"
#include "CameraControl.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

BEGIN_CAMERA_NAMESPACE

class nsGonkCameraControl : public nsCameraControl
{
public:
  nsGonkCameraControl(PRUint32 aCameraId, nsIThread *aCameraThread);
  ~nsGonkCameraControl();

  const char* GetParameter(const char *aKey);
  const char* GetParameterConstChar(PRUint32 aKey);
  double GetParameterDouble(PRUint32 aKey);
  void GetParameter(PRUint32 aKey, CameraRegion **aRegions, PRUint32 *aLength);
  void SetParameter(const char *aKey, const char *aValue);
  void SetParameter(PRUint32 aKey, const char *aValue);
  void SetParameter(PRUint32 aKey, double aValue);
  void SetParameter(PRUint32 aKey, CameraRegion *aRegions, PRUint32 aLength);
  void PushParameters();

  void ReceiveFrame(PRUint8 *aData, PRUint32 aLength);

protected:
  nsresult DoGetPreviewStream(GetPreviewStreamTask *aGetPreviewStream);
  nsresult DoAutoFocus(AutoFocusTask *aAutoFocus);
  nsresult DoTakePicture(TakePictureTask *aTakePicture);
  nsresult DoStartRecording(StartRecordingTask *aStartRecording);
  nsresult DoStopRecording(StopRecordingTask *aStopRecording);
  nsresult DoPushParameters(PushParametersTask *aPushParameters);
  nsresult DoPullParameters(PullParametersTask *aPullParameters);

  PRUint32                  mHwHandle;
  double                    mExposureCompensationMin;
  double                    mExposureCompensationStep;
  bool                      mDeferConfigUpdate;
  PRRWLock*                 mRwLock;
  android::CameraParameters mParams;
};

// camera driver callbacks
void ReceiveImage(nsGonkCameraControl* gc, PRUint8* aData, PRUint32 aLength);
void AutoFocusComplete(nsGonkCameraControl* gc, bool success);
void ReceiveFrame(nsGonkCameraControl* gc, PRUint8* aData, PRUint32 aLength);

END_CAMERA_NAMESPACE

#endif // DOM_CAMERA_GONKCAMERACONTROL_H

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_GONKCAMERACONTROL_H
#define DOM_CAMERA_GONKCAMERACONTROL_H


#include "prtypes.h"
#include "prrwlock.h"
#include "CameraControl.h"
#include "CameraCommon.h"


BEGIN_CAMERA_NAMESPACE

class nsGonkCameraControl : public nsCameraControl
{
 /*
  friend class GetPreviewStreamTask;
  friend class AutoFocusTask;
  friend class TakePictureTask;
  friend class StartRecordingTask;
  friend class StopRecordingTask;
  friend class SetParameterTask;
  friend class GetParameterTask;
  friend class PushParametersTask;
  friend class PullParametersTask;
 */

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

  PRUint32                        mHwHandle;
  double                          mExpsoureCompensationMin;
  double                          mExpsoureCompensationStep;
  bool                            mDeferConfigUpdate;
  PRRWLock*                       mRwLock;
  android::CameraParameters       mParams;
};

/* camera driver callbacks */
void GonkCameraReceiveImage(nsGonkCameraControl* gc, PRUint8* aData, PRUint32 aLength);
void GonkCameraAutoFocusComplete(nsGonkCameraControl* gc, bool success);
void GonkCameraReceiveFrame(nsGonkCameraControl* gc, PRUint8* aData, PRUint32 aLength);

END_CAMERA_NAMESPACE


#endif // DOM_CAMERA_GONKCAMERACONTROL_H

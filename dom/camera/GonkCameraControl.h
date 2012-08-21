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
#include "nsIDOMCameraManager.h"
#include "DOMCameraControl.h"
#include "CameraControlImpl.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

namespace mozilla {

namespace layers {
class GraphicBufferLocked;
}

class nsGonkCameraControl : public CameraControlImpl
{
public:
  nsGonkCameraControl(PRUint32 aCameraId, nsIThread* aCameraThread, nsDOMCameraControl* aDOMCameraControl, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError);
  nsresult Init();

  const char* GetParameter(const char* aKey);
  const char* GetParameterConstChar(PRUint32 aKey);
  double GetParameterDouble(PRUint32 aKey);
  void GetParameter(PRUint32 aKey, nsTArray<dom::CameraRegion>& aRegions);
  void SetParameter(const char* aKey, const char* aValue);
  void SetParameter(PRUint32 aKey, const char* aValue);
  void SetParameter(PRUint32 aKey, double aValue);
  void SetParameter(PRUint32 aKey, const nsTArray<dom::CameraRegion>& aRegions);
  nsresult PushParameters();

  void AutoFocusComplete(bool aSuccess);
  void TakePictureComplete(PRUint8* aData, PRUint32 aLength);

protected:
  ~nsGonkCameraControl();

  nsresult GetPreviewStreamImpl(GetPreviewStreamTask* aGetPreviewStream);
  nsresult StartPreviewImpl(StartPreviewTask* aStartPreview);
  nsresult StopPreviewImpl(StopPreviewTask* aStopPreview);
  nsresult AutoFocusImpl(AutoFocusTask* aAutoFocus);
  nsresult TakePictureImpl(TakePictureTask* aTakePicture);
  nsresult StartRecordingImpl(StartRecordingTask* aStartRecording);
  nsresult StopRecordingImpl(StopRecordingTask* aStopRecording);
  nsresult PushParametersImpl();
  nsresult PullParametersImpl();

  void SetPreviewSize(PRUint32 aWidth, PRUint32 aHeight);

  PRUint32                  mHwHandle;
  double                    mExposureCompensationMin;
  double                    mExposureCompensationStep;
  bool                      mDeferConfigUpdate;
  PRRWLock*                 mRwLock;
  android::CameraParameters mParams;
  PRUint32                  mWidth;
  PRUint32                  mHeight;
  
  enum {
    PREVIEW_FORMAT_UNKNOWN,
    PREVIEW_FORMAT_YUV420P,
    PREVIEW_FORMAT_YUV420SP
  };
  PRUint32                  mFormat;

  PRUint32                  mFps;
  PRUint32                  mDiscardedFrameCount;

private:
  nsGonkCameraControl(const nsGonkCameraControl&) MOZ_DELETE;
  nsGonkCameraControl& operator=(const nsGonkCameraControl&) MOZ_DELETE;
};

// camera driver callbacks
void ReceiveImage(nsGonkCameraControl* gc, PRUint8* aData, PRUint32 aLength);
void AutoFocusComplete(nsGonkCameraControl* gc, bool success);
void ReceiveFrame(nsGonkCameraControl* gc, layers::GraphicBufferLocked* aBuffer);

} // namespace mozilla

#endif // DOM_CAMERA_GONKCAMERACONTROL_H

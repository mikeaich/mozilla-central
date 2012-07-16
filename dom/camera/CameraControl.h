/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_NSCAMERACONTROL_H
#define DOM_CAMERA_NSCAMERACONTROL_H


// #include "utils/StrongPointer.h"
#include "camera/CameraParameters.h"
#include "prtypes.h"
#include "nsCOMPtr.h"
#include "nsThread.h"
#include "nsDOMFile.h"
#include "CameraPreview.h"
#include "nsIDOMCameraManager.h"

#define DOM_CAMERA_LOG_LEVEL 3
#include "CameraCommon.h"


BEGIN_CAMERA_NAMESPACE

class GetPreviewStreamTask;
class AutoFocusTask;
class TakePictureTask;
class StartRecordingTask;
class StopRecordingTask;
class SetParameterTask;
class GetParameterTask;
class PushParametersTask;
class PullParametersTask;
class ToggleModeTask;

/*
  Main camera control.
*/
class nsCameraControl : public nsICameraControl
{
  friend class GetPreviewStreamTask;
  friend class AutoFocusTask;
  friend class TakePictureTask;
  friend class StartRecordingTask;
  friend class StopRecordingTask;
  friend class SetParameterTask;
  friend class GetParameterTask;
  friend class PushParametersTask;
  friend class PullParametersTask;
  friend class ToggleModeTask;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICAMERACONTROL

  class CameraRegion
  {
  public:
    PRInt32 mTop;
    PRInt32 mLeft;
    PRInt32 mBottom;
    PRInt32 mRight;
    PRUint32 mWeight;
  };

  enum {
    CAMERA_PARAM_EFFECT,
    CAMERA_PARAM_WHITEBALANCE,
    CAMERA_PARAM_SCENEMODE,
    CAMERA_PARAM_FLASHMODE,
    CAMERA_PARAM_FOCUSMODE,
    CAMERA_PARAM_ZOOM,
    CAMERA_PARAM_METERINGAREAS,
    CAMERA_PARAM_FOCUSAREAS,
    CAMERA_PARAM_FOCALLENGTH,
    CAMERA_PARAM_FOCUSDISTANCENEAR,
    CAMERA_PARAM_FOCUSDISTANCEOPTIMUM,
    CAMERA_PARAM_FOCUSDISTANCEFAR,
    CAMERA_PARAM_EXPOSURECOMPENSATION
  };
  virtual const char* GetParameter(const char *aKey) = 0;
  virtual const char* GetParameterConstChar(PRUint32 aKey) = 0;
  virtual double GetParameterDouble(PRUint32 aKey) = 0;
  virtual void GetParameter(PRUint32 aKey, CameraRegion **aRegions, PRUint32 *aLength) = 0;
  virtual void SetParameter(const char *aKey, const char *aValue) = 0;
  virtual void SetParameter(PRUint32 aKey, const char *aValue) = 0;
  virtual void SetParameter(PRUint32 aKey, double aValue) = 0;
  virtual void SetParameter(PRUint32 aKey, CameraRegion *aRegions, PRUint32 aLength) = 0;
  virtual void PushParameters() = 0;

  nsCameraControl(PRUint32 aCameraId, nsIThread *aCameraThread)
    : mCameraId(aCameraId)
    , mCameraThread(aCameraThread)
    , mCapabilities(nsnull)
    , mPreview(nsnull)
    , mFileFormat(nsnull)
    , mAutoFocusOnSuccessCb(nsnull)
    , mAutoFocusOnErrorCb(nsnull)
    , mTakePictureOnSuccessCb(nsnull)
    , mTakePictureOnErrorCb(nsnull)
    , mStartRecordingOnSuccessCb(nsnull)
    , mStartRecordingOnErrorCb(nsnull)
    , mOnShutterCb(nsnull)
  { }

  virtual ~nsCameraControl()
  {
    if (mFileFormat) {
      nsMemory::Free(const_cast<char*>(mFileFormat));
    }
  }

  void TakePictureComplete(PRUint8 *aData, PRUint32 aLength);
  void AutoFocusComplete(bool aSuccess);

protected:
  virtual nsresult DoGetPreviewStream(GetPreviewStreamTask *aGetPreviewStream) = 0;
  virtual nsresult DoAutoFocus(AutoFocusTask *aAutoFocus) = 0;
  virtual nsresult DoTakePicture(TakePictureTask *aTakePicture) = 0;
  virtual nsresult DoStartRecording(StartRecordingTask *aStartRecording) = 0;
  virtual nsresult DoStopRecording(StopRecordingTask *aStopRecording) = 0;
  virtual nsresult DoPushParameters(PushParametersTask *aPushParameters) = 0;
  virtual nsresult DoPullParameters(PullParametersTask *aPullParameters) = 0;
  virtual nsresult DoToggleMode(ToggleModeTask *aToggleMode) = 0;

private:
  nsCameraControl(const nsCameraControl&);
  nsCameraControl& operator=(const nsCameraControl&);

protected:
  /* additional members */
  PRUint32                        mCameraId;
  nsCOMPtr<nsIThread>             mCameraThread;
  nsCOMPtr<nsICameraCapabilities> mCapabilities;
  PRUint32                        mPreviewWidth;
  PRUint32                        mPreviewHeight;
  nsCOMPtr<CameraPreview>         mPreview;
  const char*                     mFileFormat;

  nsCOMPtr<nsICameraAutoFocusCallback>      mAutoFocusOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback>          mAutoFocusOnErrorCb;
  nsCOMPtr<nsICameraTakePictureCallback>    mTakePictureOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback>          mTakePictureOnErrorCb;
  nsCOMPtr<nsICameraStartRecordingCallback> mStartRecordingOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback>          mStartRecordingOnErrorCb;
  nsCOMPtr<nsICameraShutterCallback>        mOnShutterCb;
};

/*
  Return the resulting preview stream to JS.  Runs on the main thread.
*/
class GetPreviewStreamResult : public nsRunnable
{
public:
  GetPreviewStreamResult(nsIDOMMediaStream *aStream, nsICameraPreviewStreamCallback *onSuccess)
    : mStream(aStream)
    , mOnSuccessCb(onSuccess)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mStream);
    }
    return NS_OK;
  }

protected:
  nsCOMPtr<nsIDOMMediaStream> mStream;
  nsCOMPtr<nsICameraPreviewStreamCallback> mOnSuccessCb;
};

/*
  Get the desired preview stream.
*/
class GetPreviewStreamTask : public nsRunnable
{
public:
  GetPreviewStreamTask(nsCameraControl *aCameraControl, PRUint32 aWidth, PRUint32 aHeight, nsICameraPreviewStreamCallback *onSuccess, nsICameraErrorCallback *onError)
    : mWidth(aWidth)
    , mHeight(aHeight)
    , mCameraControl(aCameraControl)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  { }

  NS_IMETHOD Run()
  {
    nsresult rv = mCameraControl->DoGetPreviewStream(this);

    if (NS_FAILED(rv)) {
      if (NS_FAILED(NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE"))))) {
        NS_WARNING("Failed to dispatch getPreviewStream() onError callback to main thread!");
      }
    }
    return NS_OK;
  }

  PRUint32 mWidth;
  PRUint32 mHeight;
  nsCOMPtr<nsCameraControl> mCameraControl;
  nsCOMPtr<nsICameraPreviewStreamCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};

/*
  Return the autofocus status to JS.  Runs on the main thread.
*/
class AutoFocusResult : public nsRunnable
{
public:
  AutoFocusResult(bool aSuccess, nsICameraAutoFocusCallback *onSuccess)
    : mSuccess(aSuccess)
    , mOnSuccessCb(onSuccess)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mSuccess);
    }
    return NS_OK;
  }

protected:
  bool mSuccess;
  nsCOMPtr<nsICameraAutoFocusCallback> mOnSuccessCb;
};

/*
  Autofocus the camera.
*/
class AutoFocusTask : public nsRunnable
{
public:
  AutoFocusTask(nsCameraControl *aCameraControl, nsICameraAutoFocusCallback *onSuccess, nsICameraErrorCallback *onError)
    : mCameraControl(aCameraControl)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->DoAutoFocus(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv)) {
      if (NS_FAILED(NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE"))))) {
        NS_WARNING("Failed to dispatch takePicture() onError callback to main thread!");
      }
    }
    return NS_OK;
  }

  nsCOMPtr<nsCameraControl> mCameraControl;
  nsCOMPtr<nsICameraAutoFocusCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};

/*
  Return the captured picture to JS.  Runs on the main thread.
*/
class TakePictureResult : public nsRunnable
{
public:
  TakePictureResult(nsIDOMBlob *aImage, nsICameraTakePictureCallback *onSuccess)
    : mImage(aImage)
    , mOnSuccessCb(onSuccess)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mImage);
    }
    return NS_OK;
  }

protected:
  nsCOMPtr<nsIDOMBlob> mImage;
  nsCOMPtr<nsICameraTakePictureCallback> mOnSuccessCb;
};

/*
  Capture a still image with the camera.
*/
class TakePictureTask : public nsRunnable
{
public:
  TakePictureTask(nsCameraControl *aCameraControl, PRUint32 aWidth, PRUint32 aHeight, PRInt32 aRotation, nsString aFileFormat, double aLatitude, bool aLatitudeSet, double aLongitude, bool aLongitudeSet, double aAltitude, bool aAltitudeSet, double aTimestamp, bool aTimestampSet, nsICameraTakePictureCallback *onSuccess, nsICameraErrorCallback *onError)
    : mCameraControl(aCameraControl)
    , mWidth(aWidth)
    , mHeight(aHeight)
    , mRotation(aRotation)
    , mFileFormat(aFileFormat)
    , mLatitude(aLatitude)
    , mLatitudeSet(aLatitudeSet)
    , mLongitude(aLongitude)
    , mLongitudeSet(aLongitudeSet)
    , mAltitude(aAltitude)
    , mAltitudeSet(aAltitudeSet)
    , mTimestamp(aTimestamp)
    , mTimestampSet(aTimestampSet)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->DoTakePicture(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv)) {
      if (NS_FAILED(NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE"))))) {
        NS_WARNING("Failed to dispatch takePicture() onError callback to main thread!");
      }
    }
    return NS_OK;
  }

  nsCOMPtr<nsCameraControl> mCameraControl;
  PRUint32 mWidth;
  PRUint32 mHeight;
  PRInt32 mRotation;
  nsString mFileFormat;
  double mLatitude;
  bool mLatitudeSet;
  double mLongitude;
  bool mLongitudeSet;
  double mAltitude;
  bool mAltitudeSet;
  double mTimestamp;
  bool mTimestampSet;
  nsCOMPtr<nsICameraTakePictureCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};

/*
  Return the resulting preview stream to JS.  Runs on the main thread.
*/
class ToggleModeResult : public nsRunnable
{
public:
  ToggleModeResult(nsIDOMMediaStream *aStream, nsICameraPreviewStreamCallback *onSuccess)
     : mStream(aStream)
     , mOnSuccessCb(onSuccess)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mStream);
    }
    return NS_OK;
  }

protected:
  nsCOMPtr<nsIDOMMediaStream> mStream;
  nsCOMPtr<nsICameraPreviewStreamCallback> mOnSuccessCb;
};

/*
  Get the video mode preview stream.
*/
class ToggleModeTask : public nsRunnable
{
public:
  ToggleModeTask(nsCameraControl *aCameraControl, nsICameraPreviewStreamCallback *onSuccess, nsICameraErrorCallback *onError)
    : mCameraControl(aCameraControl)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  { }

  NS_IMETHOD Run()
  {
    nsresult rv = mCameraControl->DoToggleMode(this);

    if (NS_FAILED(rv)) {
      if (NS_FAILED(NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE"))))) {
        NS_WARNING("Failed to dispatch ToggleMode() onError callback to main thread!");
      }
    }
    return NS_OK;
  }

  nsCOMPtr<nsCameraControl> mCameraControl;
  nsCOMPtr<nsICameraPreviewStreamCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};

/*
  Return the captured video to JS.  Runs on the main thread.
*/
class StartRecordingResult : public nsRunnable
{
public:
  StartRecordingResult(nsAString& aVideoFile, nsICameraStartRecordingCallback *onSuccess)
    : mVideoFile(aVideoFile)
    , mOnSuccessCb(onSuccess)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mVideoFile);
    }
    return NS_OK;
  }

protected:
  nsString mVideoFile;
  nsCOMPtr<nsICameraStartRecordingCallback> mOnSuccessCb;
};

/*
  Start video recording.
*/
class StartRecordingTask : public nsRunnable
{
public:
  StartRecordingTask(nsCameraControl *aCameraControl, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aRotation, nsICameraStartRecordingCallback *onSuccess, nsICameraErrorCallback *onError)
    : mWidth(aWidth)
    , mHeight(aHeight)
    , mRotation(aRotation)
    , mCameraControl(aCameraControl)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->DoStartRecording(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv)) {
      if (NS_FAILED(NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE"))))) {
        NS_WARNING("Failed to dispatch startRecording() onError callback to main thread!");
      }
    }
    return NS_OK;
  }

  PRUint32 mWidth;
  PRUint32 mHeight;
  PRUint32 mRotation;
  nsCOMPtr<nsCameraControl> mCameraControl;
  nsCOMPtr<nsICameraStartRecordingCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};

/*
  Stop video recording.
*/
class StopRecordingTask : public nsRunnable
{
public:
  StopRecordingTask(nsCameraControl *aCameraControl)
    : mCameraControl(aCameraControl)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->DoStopRecording(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to dispatch stopRecording()!");
    }
    return NS_OK;
  }

  nsCOMPtr<nsCameraControl> mCameraControl;
};

/*
  Pushes all camera parameters to the camera.
*/
class PushParametersTask : public nsRunnable
{
public:
  PushParametersTask(nsCameraControl *aCameraControl)
    : mCameraControl(aCameraControl)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->DoPushParameters(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to dispatch setParameter() to camera!");
    }
    return NS_OK;
  }

  nsCOMPtr<nsCameraControl> mCameraControl;
};

/*
  Get all camera parameters from the camera.
*/
class PullParametersTask : public nsRunnable
{
public:
  PullParametersTask(nsCameraControl *aCameraControl)
    : mCameraControl(aCameraControl)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->DoPullParameters(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to dispatch getParameter() to camera!");
    }
    return NS_OK;
  }

  nsCOMPtr<nsCameraControl> mCameraControl;
};

END_CAMERA_NAMESPACE


#endif // DOM_CAMERA_NSCAMERACONTROL_H

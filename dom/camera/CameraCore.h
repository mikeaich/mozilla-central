/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_CAMERACONTROLIMPL_H
#define DOM_CAMERA_CAMERACONTROLIMPL_H

#include "nsCOMPtr.h"
#include "DictionaryHelpers.h"
#include "nsIDOMCameraManager.h"

namespace mozilla {

class GetPreviewStreamTask;
class AutoFocusTask;
class TakePictureTask;
class StartRecordingTask;
class StopRecordingTask;
class SetParameterTask;
class GetParameterTask;
class PushParametersTask;
class PullParametersTask;

class CameraCore
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

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CameraCore)

  CameraCore(PRUint32 aCameraId, nsIThread* aCameraThread)
    : mCameraId(aCameraId)
    , mCameraThread(aCameraThread)
    , mFileFormat()
    , mMaxMeteringAreas(0)
    , mMaxFocusAreas(0)
    , mAutoFocusOnSuccessCb(nullptr)
    , mAutoFocusOnErrorCb(nullptr)
    , mTakePictureOnSuccessCb(nullptr)
    , mTakePictureOnErrorCb(nullptr)
    , mStartRecordingOnSuccessCb(nullptr)
    , mStartRecordingOnErrorCb(nullptr)
    , mOnShutterCb(nullptr)
  {
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  nsresult GetPreviewStream(CameraSize aSize, nsICameraPreviewStreamCallback* onSuccess, nsICameraErrorCallback* onError);
  nsresult AutoFocus(nsICameraAutoFocusCallback* onSuccess, nsICameraErrorCallback* onError);
  nsresult TakePicture(CameraSize aSize, PRInt32 aRotation, const nsAString& aFileFormat, CameraPosition aPosition, nsICameraTakePictureCallback* onSuccess, nsICameraErrorCallback* onError);
  nsresult StartRecording(CameraSize aSize, nsICameraStartRecordingCallback* onSuccess, nsICameraErrorCallback* onError);
  nsresult StopRecording();
  nsresult PushParameters();
  nsresult PullParameters();
  void Shutdown();

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
    CAMERA_PARAM_EXPOSURECOMPENSATION,

    CAMERA_PARAM_SUPPORTED_PREVIEWSIZES,
    CAMERA_PARAM_SUPPORTED_VIDEOSIZES,
    CAMERA_PARAM_SUPPORTED_PICTURESIZES,
    CAMERA_PARAM_SUPPORTED_PICTUREFORMATS,
    CAMERA_PARAM_SUPPORTED_WHITEBALANCES,
    CAMERA_PARAM_SUPPORTED_SCENEMODES,
    CAMERA_PARAM_SUPPORTED_EFFECTS,
    CAMERA_PARAM_SUPPORTED_FLASHMODES,
    CAMERA_PARAM_SUPPORTED_FOCUSMODES,
    CAMERA_PARAM_SUPPORTED_MAXFOCUSAREAS,
    CAMERA_PARAM_SUPPORTED_MAXMETERINGAREAS,
    CAMERA_PARAM_SUPPORTED_MINEXPOSURECOMPENSATION,
    CAMERA_PARAM_SUPPORTED_MAXEXPOSURECOMPENSATION,
    CAMERA_PARAM_SUPPORTED_EXPOSURECOMPENSATIONSTEP,
    CAMERA_PARAM_SUPPORTED_ZOOM,
    CAMERA_PARAM_SUPPORTED_ZOOMRATIOS
  };
  virtual const char* GetParameter(const char* aKey) = 0;
  virtual const char* GetParameterConstChar(PRUint32 aKey) = 0;
  virtual double GetParameterDouble(PRUint32 aKey) = 0;
  virtual void GetParameter(PRUint32 aKey, nsTArray<CameraRegion>& aRegions) = 0;
  virtual void SetParameter(const char* aKey, const char* aValue) = 0;
  virtual void SetParameter(PRUint32 aKey, const char* aValue) = 0;
  virtual void SetParameter(PRUint32 aKey, double aValue) = 0;
  virtual void SetParameter(PRUint32 aKey, const nsTArray<CameraRegion>& aRegions) = 0;
  virtual void PushParameters() = 0;

protected:
  virtual ~CameraCore() { }

  virtual nsresult GetPreviewStreamImpl(GetPreviewStreamTask* aGetPreviewStream) = 0;
  virtual nsresult AutoFocusImpl(AutoFocusTask* aAutoFocus) = 0;
  virtual nsresult TakePictureImpl(TakePictureTask* aTakePicture) = 0;
  virtual nsresult StartRecordingImpl(StartRecordingTask* aStartRecording) = 0;
  virtual nsresult StopRecordingImpl(StopRecordingTask* aStopRecording) = 0;
  virtual nsresult PushParametersImpl(PushParametersTask* aPushParameters) = 0;
  virtual nsresult PullParametersImpl(PullParametersTask* aPullParameters) = 0;

  PRUint32            mCameraId;
  nsCOMPtr<nsIThread> mCameraThread;
  nsString            mFileFormat;
  PRUint32            mMaxMeteringAreas;
  PRUint32            mMaxFocusAreas;

  nsCOMPtr<nsICameraAutoFocusCallback>      mAutoFocusOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback>          mAutoFocusOnErrorCb;
  nsCOMPtr<nsICameraTakePictureCallback>    mTakePictureOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback>          mTakePictureOnErrorCb;
  nsCOMPtr<nsICameraStartRecordingCallback> mStartRecordingOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback>          mStartRecordingOnErrorCb;
  nsCOMPtr<nsICameraShutterCallback>        mOnShutterCb;

private:
  CameraCore(const CameraCore&) MOZ_DELETE;
  CameraCore& operator=(const CameraCore&) MOZ_DELETE;
};

// Return the resulting preview stream to JS.  Runs on the main thread.
class GetPreviewStreamResult : public nsRunnable
{
public:
  GetPreviewStreamResult(nsIDOMMediaStream* aStream, nsICameraPreviewStreamCallback* onSuccess)
    : mStream(aStream)
    , mOnSuccessCb(onSuccess)
  {
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  ~GetPreviewStreamResult()
  {
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

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

// Get the desired preview stream.
class GetPreviewStreamTask : public nsRunnable
{
public:
  GetPreviewStreamTask(CameraCore* aCameraCore, CameraSize aSize, nsICameraPreviewStreamCallback* onSuccess, nsICameraErrorCallback* onError)
    : mSize(aSize)
    , mCameraCore(aCameraCore)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  {
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  ~GetPreviewStreamTask()
  {
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  NS_IMETHOD Run()
  {
    nsresult rv = mCameraCore->GetPreviewStreamImpl(this);

    if (NS_FAILED(rv) && mOnErrorCb) {
      rv = NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE")));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return rv;
  }

  CameraSize mSize;
  nsCOMPtr<CameraCore> mCameraCore;
  nsCOMPtr<nsICameraPreviewStreamCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};

// Return the autofocus status to JS.  Runs on the main thread.
class AutoFocusResult : public nsRunnable
{
public:
  AutoFocusResult(bool aSuccess, nsICameraAutoFocusCallback* onSuccess)
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

// Autofocus the camera.
class AutoFocusTask : public nsRunnable
{
public:
  AutoFocusTask(CameraCore* aCameraCore, nsICameraAutoFocusCallback* onSuccess, nsICameraErrorCallback* onError)
    : mCameraCore(aCameraCore)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraCore->AutoFocusImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv) && mOnErrorCb) {
      rv = NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE")));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return rv;
  }

  nsCOMPtr<CameraCore> mCameraCore;
  nsCOMPtr<nsICameraAutoFocusCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};

// Return the captured picture to JS.  Runs on the main thread.
class TakePictureResult : public nsRunnable
{
public:
  TakePictureResult(nsIDOMBlob* aImage, nsICameraTakePictureCallback* onSuccess)
    : mImage(aImage)
    , mOnSuccessCb(onSuccess)
  {
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  ~TakePictureResult()
  {
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mImage);
    }
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
    return NS_OK;
  }

protected:
  nsCOMPtr<nsIDOMBlob> mImage;
  nsCOMPtr<nsICameraTakePictureCallback> mOnSuccessCb;
};

// Capture a still image with the camera.
class TakePictureTask : public nsRunnable
{
public:
  TakePictureTask(CameraCore* aCameraCore, CameraSize aSize, PRInt32 aRotation, const nsAString& aFileFormat, CameraPosition aPosition, nsICameraTakePictureCallback* onSuccess, nsICameraErrorCallback* onError)
    : mCameraCore(aCameraCore)
    , mSize(aSize)
    , mRotation(aRotation)
    , mFileFormat(aFileFormat)
    , mPosition(aPosition)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraCore->TakePictureImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv) && mOnErrorCb) {
      rv = NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE")));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return rv;
  }

  nsCOMPtr<CameraCore> mCameraCore;
  CameraSize mSize;
  PRInt32 mRotation;
  nsString mFileFormat;
  CameraPosition mPosition;
  nsCOMPtr<nsICameraTakePictureCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};

// Return the captured video to JS.  Runs on the main thread.
class StartRecordingResult : public nsRunnable
{
public:
  StartRecordingResult(nsIDOMMediaStream* aStream, nsICameraStartRecordingCallback* onSuccess)
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
  nsCOMPtr<nsICameraStartRecordingCallback> mOnSuccessCb;
};

// Start video recording.
class StartRecordingTask : public nsRunnable
{
public:
  StartRecordingTask(CameraCore* aCameraCore, CameraSize aSize, nsICameraStartRecordingCallback* onSuccess, nsICameraErrorCallback* onError)
    : mSize(aSize)
    , mCameraCore(aCameraCore)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraCore->StartRecordingImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv) && mOnErrorCb) {
      rv = NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE")));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return rv;
  }

  CameraSize mSize;
  nsCOMPtr<CameraCore> mCameraCore;
  nsCOMPtr<nsICameraStartRecordingCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};

// Stop video recording.
class StopRecordingTask : public nsRunnable
{
public:
  StopRecordingTask(CameraCore* aCameraCore)
    : mCameraCore(aCameraCore)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraCore->StopRecordingImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  nsCOMPtr<CameraCore> mCameraCore;
};

// Pushes all camera parameters to the camera.
class PushParametersTask : public nsRunnable
{
public:
  PushParametersTask(CameraCore* aCameraCore)
    : mCameraCore(aCameraCore)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraCore->PushParametersImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  nsCOMPtr<CameraCore> mCameraCore;
};

// Get all camera parameters from the camera.
class PullParametersTask : public nsRunnable
{
public:
  PullParametersTask(CameraCore* aCameraCore)
    : mCameraCore(aCameraCore)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraCore->PullParametersImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  nsCOMPtr<CameraCore> mCameraCore;
};

} // namespace mozilla

#endif // DOM_CAMERA_CAMERACONTROLIMPL_H

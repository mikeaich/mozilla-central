/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_CAMERACONTROLIMPL_H
#define DOM_CAMERA_CAMERACONTROLIMPL_H

#include "nsCOMPtr.h"
#include "nsDOMFile.h"
#include "DictionaryHelpers.h"
#include "nsIDOMCameraManager.h"
#include "ICameraControl.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

namespace mozilla {
  
using namespace dom;

class GetPreviewStreamTask;
class StartPreviewTask;
class StopPreviewTask;
class AutoFocusTask;
class TakePictureTask;
class StartRecordingTask;
class StopRecordingTask;
class SetParameterTask;
class GetParameterTask;
class PushParametersTask;
class PullParametersTask;
  
class DOMCameraPreview;

class CameraControlImpl : public ICameraControl
{
  friend class GetPreviewStreamTask;
  friend class StartPreviewTask;
  friend class StopPreviewTask;
  friend class AutoFocusTask;
  friend class TakePictureTask;
  friend class StartRecordingTask;
  friend class StopRecordingTask;
  friend class SetParameterTask;
  friend class GetParameterTask;
  friend class PushParametersTask;
  friend class PullParametersTask;

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CameraControlImpl)

  CameraControlImpl(PRUint32 aCameraId, nsIThread* aCameraThread)
    : mCameraId(aCameraId)
    , mCameraThread(aCameraThread)
    , mFileFormat()
    , mMaxMeteringAreas(0)
    , mMaxFocusAreas(0)
    , mDOMPreview(nullptr)
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
  nsresult StartPreview(DOMCameraPreview* aDOMPreview);
  void StopPreview();
  nsresult AutoFocus(nsICameraAutoFocusCallback* onSuccess, nsICameraErrorCallback* onError);
  nsresult TakePicture(CameraSize aSize, PRInt32 aRotation, const nsAString& aFileFormat, CameraPosition aPosition, nsICameraTakePictureCallback* onSuccess, nsICameraErrorCallback* onError);
  nsresult StartRecording(CameraSize aSize, nsICameraStartRecordingCallback* onSuccess, nsICameraErrorCallback* onError);
  nsresult StopRecording();
  nsresult PushParameters();
  nsresult PullParameters();

  nsresult Set(PRUint32 aKey, const nsAString& aValue);
  nsresult Get(PRUint32 aKey, nsAString& aValue);
  nsresult Set(PRUint32 aKey, double aValue);
  nsresult Get(PRUint32 aKey, double* aValue);
  nsresult Set(JSContext* aCx, PRUint32 aKey, const JS::Value& aValue, PRUint32 aLimit);
  nsresult Get(JSContext* aCx, PRUint32 aKey, JS::Value* aValue);

  nsresult SetFocusAreas(JSContext* aCx, const JS::Value& aValue)
  {
    return Set(aCx, CAMERA_PARAM_FOCUSAREAS, aValue, mMaxFocusAreas);
  }

  nsresult SetMeteringAreas(JSContext* aCx, const JS::Value& aValue)
  {
    return Set(aCx, CAMERA_PARAM_METERINGAREAS, aValue, mMaxMeteringAreas);
  }

  virtual const char* GetParameter(const char* aKey) = 0;
  virtual const char* GetParameterConstChar(PRUint32 aKey) = 0;
  virtual double GetParameterDouble(PRUint32 aKey) = 0;
  virtual void GetParameter(PRUint32 aKey, nsTArray<CameraRegion>& aRegions) = 0;
  virtual void SetParameter(const char* aKey, const char* aValue) = 0;
  virtual void SetParameter(PRUint32 aKey, const char* aValue) = 0;
  virtual void SetParameter(PRUint32 aKey, double aValue) = 0;
  virtual void SetParameter(PRUint32 aKey, const nsTArray<CameraRegion>& aRegions) = 0;

protected:
  virtual ~CameraControlImpl() { }

  void ReceiveFrame(PRUint8* aData);

  virtual nsresult GetPreviewStreamImpl(GetPreviewStreamTask* aGetPreviewStream) = 0;
  virtual nsresult StartPreviewImpl(StartPreviewTask* aStartPreview) = 0;
  virtual nsresult StopPreviewImpl(StopPreviewTask* aStopPreview) = 0;
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

  /**
   * 'mDOMPreview' is a raw pointer to the object that will receive incoming
   * preview frames.  This is guaranteed to be valid, or null.
   *
   * It is set by a call to StartPreview(), and set to null on StopPreview().
   * It is up to the caller to ensure that the object will not disappear
   * out from under this pointer--usually by calling NS_ADDREF().
   */
  DOMCameraPreview*   mDOMPreview;

  nsCOMPtr<nsICameraAutoFocusCallback>      mAutoFocusOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback>          mAutoFocusOnErrorCb;
  nsCOMPtr<nsICameraTakePictureCallback>    mTakePictureOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback>          mTakePictureOnErrorCb;
  nsCOMPtr<nsICameraStartRecordingCallback> mStartRecordingOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback>          mStartRecordingOnErrorCb;
  nsCOMPtr<nsICameraShutterCallback>        mOnShutterCb;

private:
  CameraControlImpl(const CameraControlImpl&) MOZ_DELETE;
  CameraControlImpl& operator=(const CameraControlImpl&) MOZ_DELETE;
};

// Return the resulting preview stream to JS.  Runs on the main thread.
class GetPreviewStreamResult : public nsRunnable
{
public:
  GetPreviewStreamResult(CameraControlImpl* aCameraControl, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aFramesPerSecond, nsICameraPreviewStreamCallback* onSuccess)
    : mCameraControl(aCameraControl)
    , mWidth(aWidth)
    , mHeight(aHeight)
    , mFramesPerSecond(aFramesPerSecond)
    , mOnSuccessCb(onSuccess)
  {
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  ~GetPreviewStreamResult()
  {
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  NS_IMETHOD Run();

protected:
  nsRefPtr<CameraControlImpl> mCameraControl;
  PRUint32 mWidth;
  PRUint32 mHeight;
  PRUint32 mFramesPerSecond;
  nsCOMPtr<nsICameraPreviewStreamCallback> mOnSuccessCb;
};

// Get the desired preview stream.
class GetPreviewStreamTask : public nsRunnable
{
public:
  GetPreviewStreamTask(CameraControlImpl* aCameraControl, CameraSize aSize, nsICameraPreviewStreamCallback* onSuccess, nsICameraErrorCallback* onError)
    : mSize(aSize)
    , mCameraControl(aCameraControl)
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
    nsresult rv = mCameraControl->GetPreviewStreamImpl(this);

    if (NS_FAILED(rv) && mOnErrorCb) {
      rv = NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE")));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return rv;
  }

  CameraSize mSize;
  nsRefPtr<CameraControlImpl> mCameraControl;
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
  AutoFocusTask(CameraControlImpl* aCameraControl, nsICameraAutoFocusCallback* onSuccess, nsICameraErrorCallback* onError)
    : mCameraControl(aCameraControl)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->AutoFocusImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv) && mOnErrorCb) {
      rv = NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE")));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return rv;
  }

  nsRefPtr<CameraControlImpl> mCameraControl;
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
  TakePictureTask(CameraControlImpl* aCameraControl, CameraSize aSize, PRInt32 aRotation, const nsAString& aFileFormat, CameraPosition aPosition, nsICameraTakePictureCallback* onSuccess, nsICameraErrorCallback* onError)
    : mCameraControl(aCameraControl)
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
    nsresult rv = mCameraControl->TakePictureImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv) && mOnErrorCb) {
      rv = NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE")));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return rv;
  }

  nsRefPtr<CameraControlImpl> mCameraControl;
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
  StartRecordingTask(CameraControlImpl* aCameraControl, CameraSize aSize, nsICameraStartRecordingCallback* onSuccess, nsICameraErrorCallback* onError)
    : mSize(aSize)
    , mCameraControl(aCameraControl)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->StartRecordingImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv) && mOnErrorCb) {
      rv = NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE")));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return rv;
  }

  CameraSize mSize;
  nsRefPtr<CameraControlImpl> mCameraControl;
  nsCOMPtr<nsICameraStartRecordingCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};

// Stop video recording.
class StopRecordingTask : public nsRunnable
{
public:
  StopRecordingTask(CameraControlImpl* aCameraControl)
    : mCameraControl(aCameraControl)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->StopRecordingImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  nsRefPtr<CameraControlImpl> mCameraControl;
};

// Pushes all camera parameters to the camera.
class PushParametersTask : public nsRunnable
{
public:
  PushParametersTask(CameraControlImpl* aCameraControl)
    : mCameraControl(aCameraControl)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->PushParametersImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  nsRefPtr<CameraControlImpl> mCameraControl;
};

// Get all camera parameters from the camera.
class PullParametersTask : public nsRunnable
{
public:
  PullParametersTask(CameraControlImpl* aCameraControl)
    : mCameraControl(aCameraControl)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->PullParametersImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  nsRefPtr<CameraControlImpl> mCameraControl;
};

// Start the preview.
class StartPreviewTask : public nsRunnable
{
public:
  StartPreviewTask(CameraControlImpl* aCameraControl, DOMCameraPreview* aDOMPreview)
    : mCameraControl(aCameraControl)
    , mDOMPreview(aDOMPreview)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->StartPreviewImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  nsRefPtr<CameraControlImpl> mCameraControl;
  DOMCameraPreview* mDOMPreview; // DOMCameraPreview NS_ADDREFs itself for us
};

// Stop the preview.
class StopPreviewTask : public nsRunnable
{
public:
  StopPreviewTask(CameraControlImpl* aCameraControl)
    : mCameraControl(aCameraControl)
  { }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    mCameraControl->StopPreviewImpl(this);
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

    return NS_OK;
  }

  nsRefPtr<CameraControlImpl> mCameraControl;
};

} // namespace mozilla

#endif // DOM_CAMERA_CAMERACONTROLIMPL_H

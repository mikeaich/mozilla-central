/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DOMCameraPreview.h"
#include "CameraControl.h"

#define DOM_CAMERA_DEBUG_REFS 1
#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;

// Helpers for string properties.
nsresult
CameraControl::Set(PRUint32 aKey, const nsAString& aValue)
{
  SetParameter(aKey, NS_ConvertUTF16toUTF8(aValue).get());
  return NS_OK;
}

nsresult
CameraControl::Get(PRUint32 aKey, nsAString& aValue)
{
  const char* value = GetParameterConstChar(aKey);
  if (!value) {
    return NS_ERROR_FAILURE;
  }

  aValue.AssignASCII(value);
  return NS_OK;
}

// Helpers for doubles.
nsresult
CameraControl::Set(PRUint32 aKey, double aValue)
{
  SetParameter(aKey, aValue);
  return NS_OK;
}

nsresult
CameraControl::Get(PRUint32 aKey, double* aValue)
{
  MOZ_ASSERT(aValue);
  *aValue = GetParameterDouble(aKey);
  return NS_OK;
}

// Helper for weighted regions.
nsresult
CameraControl::Set(JSContext* aCx, PRUint32 aKey, const JS::Value& aValue, PRUint32 aLimit)
{
  if (aLimit == 0) {
    DOM_CAMERA_LOGI("%s:%d : aLimit = 0, nothing to do\n", __func__, __LINE__);
    return NS_OK;
  }

  if (!aValue.isObject()) {
    return NS_ERROR_INVALID_ARG;
  }

  uint32_t length = 0;

  JSObject* regions = &aValue.toObject();
  if (!JS_GetArrayLength(aCx, regions, &length)) {
    return NS_ERROR_FAILURE;
  }

  DOM_CAMERA_LOGI("%s:%d : got %d regions (limited to %d)\n", __func__, __LINE__, length, aLimit);
  if (length > aLimit) {
    length = aLimit;
  }
    
  nsTArray<CameraRegion> regionArray;
  regionArray.SetCapacity(length);

  for (PRUint32 i = 0; i < length; ++i) {
    JS::Value v;

    if (!JS_GetElement(aCx, regions, i, &v)) {
      return NS_ERROR_FAILURE;
    }

    CameraRegion* r = regionArray.AppendElement();
    /**
     * These are the default values.  We can remove these when the xpidl
     * dictionary parser gains the ability to grok default values.
     */
    r->top = -1000;
    r->left = -1000;
    r->bottom = 1000;
    r->right = 1000;
    r->weight = 1000;

    nsresult rv = r->Init(aCx, &v);
    NS_ENSURE_SUCCESS(rv, rv);

    DOM_CAMERA_LOGI("region %d: top=%d, left=%d, bottom=%d, right=%d, weight=%d\n",
      i,
      r->top,
      r->left,
      r->bottom,
      r->right,
      r->weight
    );
  }
  SetParameter(aKey, regionArray);
  return NS_OK;
}

nsresult
CameraControl::Get(JSContext* aCx, PRUint32 aKey, JS::Value* aValue)
{
  nsTArray<CameraRegion> regionArray;

  GetParameter(aKey, regionArray);

  JSObject* array = JS_NewArrayObject(aCx, 0, nullptr);
  if (!array) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRUint32 length = regionArray.Length();
  DOM_CAMERA_LOGI("%s:%d : got %d regions\n", __func__, __LINE__, length);

  for (PRUint32 i = 0; i < length; ++i) {
    CameraRegion* r = &regionArray[i];
    JS::Value v;

    JSObject* o = JS_NewObject(aCx, nullptr, nullptr, nullptr);
    if (!o) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    DOM_CAMERA_LOGI("top=%d\n", r->top);
    v = INT_TO_JSVAL(r->top);
    if (!JS_SetProperty(aCx, o, "top", &v)) {
      return NS_ERROR_FAILURE;
    }
    DOM_CAMERA_LOGI("left=%d\n", r->left);
    v = INT_TO_JSVAL(r->left);
    if (!JS_SetProperty(aCx, o, "left", &v)) {
      return NS_ERROR_FAILURE;
    }
    DOM_CAMERA_LOGI("bottom=%d\n", r->bottom);
    v = INT_TO_JSVAL(r->bottom);
    if (!JS_SetProperty(aCx, o, "bottom", &v)) {
      return NS_ERROR_FAILURE;
    }
    DOM_CAMERA_LOGI("right=%d\n", r->right);
    v = INT_TO_JSVAL(r->right);
    if (!JS_SetProperty(aCx, o, "right", &v)) {
      return NS_ERROR_FAILURE;
    }
    DOM_CAMERA_LOGI("weight=%d\n", r->weight);
    v = INT_TO_JSVAL(r->weight);
    if (!JS_SetProperty(aCx, o, "weight", &v)) {
      return NS_ERROR_FAILURE;
    }

    v = OBJECT_TO_JSVAL(o);
    if (!JS_SetElement(aCx, array, i, &v)) {
      return NS_ERROR_FAILURE;
    }
  }

  *aValue = JS::ObjectValue(*array);
  return NS_OK;
}

nsresult
CameraControl::GetPreviewStream(CameraSize aSize, nsICameraPreviewStreamCallback* onSuccess, nsICameraErrorCallback* onError)
{
  nsCOMPtr<nsIRunnable> getPreviewStreamTask = new GetPreviewStreamTask(this, aSize, onSuccess, onError);
  return NS_DispatchToMainThread(getPreviewStreamTask);
}

nsresult
CameraControl::AutoFocus(nsICameraAutoFocusCallback* onSuccess, nsICameraErrorCallback* onError)
{
  nsCOMPtr<nsIRunnable> autoFocusTask = new AutoFocusTask(this, onSuccess, onError);
  return mCameraThread->Dispatch(autoFocusTask, NS_DISPATCH_NORMAL);
}

nsresult
CameraControl::TakePicture(CameraSize aSize, PRInt32 aRotation, const nsAString& aFileFormat, CameraPosition aPosition, nsICameraTakePictureCallback* onSuccess, nsICameraErrorCallback* onError)
{
  nsCOMPtr<nsIRunnable> takePictureTask = new TakePictureTask(this, aSize, aRotation, aFileFormat, aPosition, onSuccess, onError);
  return mCameraThread->Dispatch(takePictureTask, NS_DISPATCH_NORMAL);
}

nsresult
CameraControl::StartRecording(CameraSize aSize, nsICameraStartRecordingCallback* onSuccess, nsICameraErrorCallback* onError)
{
  nsCOMPtr<nsIRunnable> startRecordingTask = new StartRecordingTask(this, aSize, onSuccess, onError);
  return mCameraThread->Dispatch(startRecordingTask, NS_DISPATCH_NORMAL);
}

nsresult
CameraControl::StopRecording()
{
  nsCOMPtr<nsIRunnable> stopRecordingTask = new StopRecordingTask(this);
  return mCameraThread->Dispatch(stopRecordingTask, NS_DISPATCH_NORMAL);
}

nsresult
CameraControl::PushParameters()
{
  return NS_ERROR_NOT_IMPLEMENTED; // TODO
}

nsresult
CameraControl::PullParameters()
{
  return NS_ERROR_NOT_IMPLEMENTED; // TODO
}

nsresult
CameraControl::StartPreview(DOMCameraPreview* aDOMPreview)
{
  NS_ENSURE_TRUE(aDOMPreview, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIRunnable> startPreviewTask = new StartPreviewTask(this, aDOMPreview);
  return mCameraThread->Dispatch(startPreviewTask, NS_DISPATCH_NORMAL);
}

void
CameraControl::StopPreview()
{
  nsCOMPtr<nsIRunnable> stopPreviewTask = new StopPreviewTask(this);
  mCameraThread->Dispatch(stopPreviewTask, NS_DISPATCH_NORMAL);
}

void
CameraControl::ReceiveFrame(PRUint8* aData)
{
  if (mDOMPreview) {
    mDOMPreview->ReceiveFrame(aData);
  }
}

NS_IMETHODIMP
GetPreviewStreamResult::Run()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mOnSuccessCb) {
    nsCOMPtr<nsIDOMMediaStream> stream = new DOMCameraPreview(mCameraControl, mWidth, mHeight, mFramesPerSecond);
    mOnSuccessCb->HandleEvent(stream);
  }
  return NS_OK;
}

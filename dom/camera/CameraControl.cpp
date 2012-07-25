/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsCOMPtr.h"
#include "nsDOMClassInfo.h"
#include "jsapi.h"
#include "nsThread.h"
#include "DOMCameraManager.h"
#include "CameraControl.h"
#include "CameraCapabilities.h"
#include "CameraControl.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;
using namespace dom;

DOMCI_DATA(CameraControl, nsICameraControl)

NS_INTERFACE_MAP_BEGIN(nsCameraControl)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsICameraControl)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CameraControl)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsCameraControl)
NS_IMPL_RELEASE(nsCameraControl)

// Helpers for string properties.
static inline nsresult
setHelper(nsCameraControl *aCameraContol, PRUint32 aKey, const nsAString& aValue)
{
  aCameraContol->SetParameter(aKey, NS_ConvertUTF16toUTF8(aValue).get());
  return NS_OK;
}

static nsresult
getHelper(nsCameraControl *aCameraControl, PRUint32 aKey, nsAString& aValue)
{
  const char *value = aCameraControl->GetParameterConstChar(aKey);
  if (!value) {
    return NS_ERROR_FAILURE;
  }

  aValue.AssignASCII(value);
  return NS_OK;
}

// Helpers for doubles.
static inline nsresult
setHelper(nsCameraControl *aCameraContol, PRUint32 aKey, double aValue)
{
  aCameraContol->SetParameter(aKey, aValue);
  return NS_OK;
}

static inline nsresult
getHelper(nsCameraControl *aCameraControl, PRUint32 aKey, double *aValue)
{
  MOZ_ASSERT(aValue);
  *aValue = aCameraControl->GetParameterDouble(aKey);
  return NS_OK;
}

// Helper for weighted regions.
static nsresult
setHelper(nsCameraControl *aCameraContol, PRUint32 aKey, const JS::Value & aValue, JSContext *cx, PRUint32 aLimit)
{
#if 0
  if (aLimit == 0) {
    DOM_CAMERA_LOGI("%s:%d : aLimit = 0, nothing to do\n", __func__, __LINE__);
    return NS_OK;
  }
#endif

  PRUint32 length = 0;

  if (aValue.isObject()) {
    JSObject *regions = JSVAL_TO_OBJECT(aValue);
    if (!JS_GetArrayLength(cx, regions, &length)) {
      return NS_ERROR_FAILURE;
    }

    DOM_CAMERA_LOGI("%s:%d : got %d regions (limited to %d)\n", __func__, __LINE__, length, aLimit);
#if 0
    if (length > aLimit) {
      length = aLimit;
    }
#endif
      
    nsTArray<CameraRegion> regionArray;
    regionArray.SetCapacity(length);

    for (PRUint32 i = 0; i < length; ++i) {
      jsval v;

      if (!JS_GetElement(cx, regions, i, &v)) {
        return NS_ERROR_FAILURE;
      }

      CameraRegion *r = regionArray.AppendElement();
      /**
       * These are the default values.  We can remove these when the xpidl
       * dictionary parser gains the ability to grok default values.
       */
      r->top = -1000;
      r->left = -1000;
      r->bottom = 1000;
      r->right = 1000;
      r->weight = 1000;

      nsresult rv = r->Init(cx, &v);
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
    aCameraContol->SetParameter(aKey, regionArray);
  }

  return NS_OK;
}

static nsresult
getHelper(nsCameraControl *aCameraControl, PRUint32 aKey, JSContext *cx, JS::Value *aValue)
{
  nsTArray<CameraRegion> regionArray;

  aCameraControl->GetParameter(aKey, regionArray);

  JSObject *array = JS_NewArrayObject(cx, 0, nsnull);
  if (!array) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  PRUint32 length = regionArray.Length();
  DOM_CAMERA_LOGI("%s:%d : got %d regions\n", __func__, __LINE__, length);

  for (PRUint32 i = 0; i < length; ++i) {
    CameraRegion *r = &regionArray[i];
    JS::Value v;

    JSObject *o = JS_NewObject(cx, nsnull, nsnull, nsnull);
    if (!o) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    DOM_CAMERA_LOGI("top=%d\n", r->top);
    v = INT_TO_JSVAL(r->top);
    if (!JS_SetProperty(cx, o, "top", &v)) {
      return NS_ERROR_FAILURE;
    }
    DOM_CAMERA_LOGI("left=%d\n", r->left);
    v = INT_TO_JSVAL(r->left);
    if (!JS_SetProperty(cx, o, "left", &v)) {
      return NS_ERROR_FAILURE;
    }
    DOM_CAMERA_LOGI("bottom=%d\n", r->bottom);
    v = INT_TO_JSVAL(r->bottom);
    if (!JS_SetProperty(cx, o, "bottom", &v)) {
      return NS_ERROR_FAILURE;
    }
    DOM_CAMERA_LOGI("right=%d\n", r->right);
    v = INT_TO_JSVAL(r->right);
    if (!JS_SetProperty(cx, o, "right", &v)) {
      return NS_ERROR_FAILURE;
    }
    DOM_CAMERA_LOGI("weight=%d\n", r->weight);
    v = INT_TO_JSVAL(r->weight);
    if (!JS_SetProperty(cx, o, "weight", &v)) {
      return NS_ERROR_FAILURE;
    }

    v = OBJECT_TO_JSVAL(o);
    if (!JS_SetElement(cx, array, i, &v)) {
      return NS_ERROR_FAILURE;
    }
  }

  *aValue = JS::ObjectValue(*array);
  return NS_OK;
}

/* readonly attribute nsICameraCapabilities capabilities; */
NS_IMETHODIMP
nsCameraControl::GetCapabilities(nsICameraCapabilities * *aCapabilities)
{
  nsCOMPtr<nsICameraCapabilities> capabilities = mCapabilities;

  if (!capabilities) {
    capabilities = new nsCameraCapabilities(this);
    mCapabilities = capabilities;
  }

  capabilities.forget(aCapabilities);
  return NS_OK;
}

/* attribute DOMString effect; */
NS_IMETHODIMP
nsCameraControl::GetEffect(nsAString & aEffect)
{
  return getHelper(this, CAMERA_PARAM_EFFECT, aEffect);
}
NS_IMETHODIMP
nsCameraControl::SetEffect(const nsAString & aEffect)
{
  return setHelper(this, CAMERA_PARAM_EFFECT, aEffect);
}

/* attribute DOMString whiteBalanceMode; */
NS_IMETHODIMP
nsCameraControl::GetWhiteBalanceMode(nsAString & aWhiteBalanceMode)
{
  return getHelper(this, CAMERA_PARAM_WHITEBALANCE, aWhiteBalanceMode);
}
NS_IMETHODIMP
nsCameraControl::SetWhiteBalanceMode(const nsAString & aWhiteBalanceMode)
{
  return setHelper(this, CAMERA_PARAM_WHITEBALANCE, aWhiteBalanceMode);
}

/* attribute DOMString sceneMode; */
NS_IMETHODIMP
nsCameraControl::GetSceneMode(nsAString & aSceneMode)
{
  return getHelper(this, CAMERA_PARAM_SCENEMODE, aSceneMode);
}
NS_IMETHODIMP
nsCameraControl::SetSceneMode(const nsAString & aSceneMode)
{
  return setHelper(this, CAMERA_PARAM_SCENEMODE, aSceneMode);
}

/* attribute DOMString flashMode; */
NS_IMETHODIMP
nsCameraControl::GetFlashMode(nsAString & aFlashMode)
{
  return getHelper(this, CAMERA_PARAM_FLASHMODE, aFlashMode);
}
NS_IMETHODIMP
nsCameraControl::SetFlashMode(const nsAString & aFlashMode)
{
  return setHelper(this, CAMERA_PARAM_FLASHMODE, aFlashMode);
}

/* attribute DOMString focusMode; */
NS_IMETHODIMP
nsCameraControl::GetFocusMode(nsAString & aFocusMode)
{
  return getHelper(this, CAMERA_PARAM_FOCUSMODE, aFocusMode);
}
NS_IMETHODIMP
nsCameraControl::SetFocusMode(const nsAString & aFocusMode)
{
  return setHelper(this, CAMERA_PARAM_FOCUSMODE, aFocusMode);
}

/* attribute double zoom; */
NS_IMETHODIMP
nsCameraControl::GetZoom(double *aZoom)
{
  return getHelper(this, CAMERA_PARAM_ZOOM, aZoom);
}
NS_IMETHODIMP
nsCameraControl::SetZoom(double aZoom)
{
  return setHelper(this, CAMERA_PARAM_ZOOM, aZoom);
}

/* attribute jsval meteringAreas; */
NS_IMETHODIMP
nsCameraControl::GetMeteringAreas(JSContext *cx, JS::Value *aMeteringAreas)
{
  return getHelper(this, CAMERA_PARAM_METERINGAREAS, cx, aMeteringAreas);
}
NS_IMETHODIMP
nsCameraControl::SetMeteringAreas(JSContext *cx, const JS::Value & aMeteringAreas)
{
  return setHelper(this, CAMERA_PARAM_METERINGAREAS, aMeteringAreas, cx, mMaxMeteringAreas);
}

/* attribute jsval focusAreas; */
NS_IMETHODIMP
nsCameraControl::GetFocusAreas(JSContext *cx, JS::Value *aFocusAreas)
{
  return getHelper(this, CAMERA_PARAM_FOCUSAREAS, cx, aFocusAreas);
}
NS_IMETHODIMP
nsCameraControl::SetFocusAreas(JSContext *cx, const JS::Value & aFocusAreas)
{
  return setHelper(this, CAMERA_PARAM_FOCUSAREAS, aFocusAreas, cx, mMaxFocusAreas);
}

/* readonly attribute double focalLength; */
NS_IMETHODIMP
nsCameraControl::GetFocalLength(double *aFocalLength)
{
  return getHelper(this, CAMERA_PARAM_FOCALLENGTH, aFocalLength);
}

/* readonly attribute double focusDistanceNear; */
NS_IMETHODIMP
nsCameraControl::GetFocusDistanceNear(double *aFocusDistanceNear)
{
  return getHelper(this, CAMERA_PARAM_FOCUSDISTANCENEAR, aFocusDistanceNear);
}

/* readonly attribute double focusDistanceOptimum; */
NS_IMETHODIMP
nsCameraControl::GetFocusDistanceOptimum(double *aFocusDistanceOptimum)
{
  return getHelper(this, CAMERA_PARAM_FOCUSDISTANCEOPTIMUM, aFocusDistanceOptimum);
}

/* readonly attribute double focusDistanceFar; */
NS_IMETHODIMP
nsCameraControl::GetFocusDistanceFar(double *aFocusDistanceFar)
{
  return getHelper(this, CAMERA_PARAM_FOCUSDISTANCEFAR, aFocusDistanceFar);
}

/* void setExposureCompensation (const JS::Value & aCompensation); */
NS_IMETHODIMP
nsCameraControl::SetExposureCompensation(const JS::Value & aCompensation)
{
  if (JSVAL_IS_DOUBLE(aCompensation)) {
    double compensation = JSVAL_TO_DOUBLE(aCompensation);
    return setHelper(this, CAMERA_PARAM_EXPOSURECOMPENSATION, compensation);
  }
  if (JSVAL_IS_INT(aCompensation)) {
    PRUint32 compensation = JSVAL_TO_INT(aCompensation);
    return setHelper(this, CAMERA_PARAM_EXPOSURECOMPENSATION, compensation);
  }
  if (JSVAL_IS_NULL(aCompensation) || JSVAL_IS_VOID(aCompensation)) {
    /* use NaN to switch the camera back into auto mode */
    return setHelper(this, CAMERA_PARAM_EXPOSURECOMPENSATION, NAN);
  }
  return NS_ERROR_INVALID_ARG;
}

/* readonly attribute double exposureCompensation; */
NS_IMETHODIMP
nsCameraControl::GetExposureCompensation(double *aExposureCompensation)
{
  return getHelper(this, CAMERA_PARAM_EXPOSURECOMPENSATION, aExposureCompensation);
}

/* attribute nsICameraShutterCallback onShutter; */
NS_IMETHODIMP
nsCameraControl::GetOnShutter(nsICameraShutterCallback * *aOnShutter)
{
  *aOnShutter = mOnShutterCb;
  return NS_OK;
}
NS_IMETHODIMP
nsCameraControl::SetOnShutter(nsICameraShutterCallback *aOnShutter)
{
  mOnShutterCb = aOnShutter;
  return NS_OK;
}

/* void startRecording (in jsval aOptions, in nsICameraStartRecordingCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP
nsCameraControl::StartRecording(const JS::Value & aOptions, nsICameraStartRecordingCallback *onSuccess, nsICameraErrorCallback *onError, JSContext* cx)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  CameraSize size;
  nsresult rv = size.Init(cx, &aOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> startRecordingTask = new StartRecordingTask(this, size, onSuccess, onError);
  mCameraThread->Dispatch(startRecordingTask, NS_DISPATCH_NORMAL);

  return NS_OK;
}

/* void stopRecording (); */
NS_IMETHODIMP
nsCameraControl::StopRecording()
{
  nsCOMPtr<nsIRunnable> stopRecordingTask = new StopRecordingTask(this);
  mCameraThread->Dispatch(stopRecordingTask, NS_DISPATCH_NORMAL);

  return NS_OK;
}

/* [implicit_jscontext] void getPreviewStream (in jsval aOptions, in nsICameraPreviewStreamCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP
nsCameraControl::GetPreviewStream(const JS::Value & aOptions, nsICameraPreviewStreamCallback *onSuccess, nsICameraErrorCallback *onError, JSContext* cx)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  CameraSize size;
  nsresult rv = size.Init(cx, &aOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> getPreviewStreamTask = new GetPreviewStreamTask(this, size, onSuccess, onError);
  mCameraThread->Dispatch(getPreviewStreamTask, NS_DISPATCH_NORMAL);

  return NS_OK;
}

/* void autoFocus (in nsICameraAutoFocusCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP
nsCameraControl::AutoFocus(nsICameraAutoFocusCallback *onSuccess, nsICameraErrorCallback *onError)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIRunnable> autoFocusTask = new AutoFocusTask(this, onSuccess, onError);
  mCameraThread->Dispatch(autoFocusTask, NS_DISPATCH_NORMAL);

  return NS_OK;
}

/* void takePicture (in nsICameraTakePictureCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP nsCameraControl::TakePicture(nsICameraPictureOptions *aOptions, nsICameraTakePictureCallback *onSuccess, nsICameraErrorCallback *onError, JSContext* cx)
{
  PRInt32 rotation = 0;

  CameraSize size;
  CameraPosition pos;

  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);
  NS_ENSURE_TRUE(aOptions, NS_ERROR_INVALID_ARG);

  jsval pictureSize;
  nsresult rv = aOptions->GetPictureSize(&pictureSize);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = size.Init(cx, &pictureSize);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString fileFormat;
  rv = aOptions->GetFileFormat(fileFormat);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aOptions->GetRotation(&rotation);
  NS_ENSURE_SUCCESS(rv, rv);

  jsval position;
  rv = aOptions->GetPosition(&position);
  NS_ENSURE_SUCCESS(rv, rv);

  /**
   * Default values, until the dictionary parser can handle them.
   * NaN indicates no value provided.
   */
  pos.latitude = NAN;
  pos.longitude = NAN;
  pos.altitude = NAN;
  pos.timestamp = NAN;
  rv = pos.Init(cx, &position);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> takePictureTask = new TakePictureTask(this, size, rotation, fileFormat, pos, onSuccess, onError);
  mCameraThread->Dispatch(takePictureTask, NS_DISPATCH_NORMAL);

  return NS_OK;
}

void
nsCameraControl::AutoFocusComplete(bool aSuccess)
{
  /**
   * Auto focusing can change some of the camera's parameters, so
   * we need to pull a new set before sending the result to the
   * main thread.
   */
  PullParametersImpl(nsnull);

  nsCOMPtr<nsIRunnable> autoFocusResult = new AutoFocusResult(aSuccess, mAutoFocusOnSuccessCb);

  nsresult rv = NS_DispatchToMainThread(autoFocusResult);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch autoFocus() onSuccess callback to main thread!");
  }
}

void
nsCameraControl::TakePictureComplete(PRUint8* aData, PRUint32 aLength)
{
  PRUint8* data = new PRUint8[aLength];

  memcpy(data, aData, aLength);

  /**
   * TODO: pick up the actual specified picture format for the MIME type;
   * for now, assume we'll be using JPEGs.
   */
  nsIDOMBlob *blob = new nsDOMMemoryFile((void*)data, (PRUint64)aLength, NS_LITERAL_STRING("image/jpeg"));
  nsCOMPtr<nsIRunnable> takePictureResult = new TakePictureResult(blob, mTakePictureOnSuccessCb);

  nsresult rv = NS_DispatchToMainThread(takePictureResult);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch takePicture() onSuccess callback to main thread!");
  }
}

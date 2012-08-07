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

#define DOM_CAMERA_DEBUG_REFS 1
#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;
using namespace dom;

DOMCI_DATA(CameraControl, nsICameraControl)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsCameraControl)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsCameraControl)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCameraThread)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCapabilities)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPreview)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsCameraControl)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCameraThread)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCapabilities)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mPreview)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsCameraControl)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsICameraControl)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CameraControl)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsCameraControl)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsCameraControl)

// Helpers for string properties.
nsresult
nsCameraControl::SetHelper(PRUint32 aKey, const nsAString& aValue)
{
  mCameraCore->SetParameter(aKey, NS_ConvertUTF16toUTF8(aValue).get());
  return NS_OK;
}

nsresult
nsCameraControl::GetHelper(PRUint32 aKey, nsAString& aValue)
{
  const char* value = mCameraCore->GetParameterConstChar(aKey);
  if (!value) {
    return NS_ERROR_FAILURE;
  }

  aValue.AssignASCII(value);
  return NS_OK;
}

// Helpers for doubles.
nsresult
nsCameraControl::SetHelper(PRUint32 aKey, double aValue)
{
  mCameraCore->SetParameter(aKey, aValue);
  return NS_OK;
}

nsresult
nsCameraControl::GetHelper(PRUint32 aKey, double* aValue)
{
  MOZ_ASSERT(aValue);
  *aValue = mCameraCore->GetParameterDouble(aKey);
  return NS_OK;
}

// Helper for weighted regions.
nsresult
nsCameraControl::SetHelper(JSContext* aCx, PRUint32 aKey, const JS::Value& aValue, PRUint32 aLimit)
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
  mCameraCore->SetParameter(aKey, regionArray);
  return NS_OK;
}

nsresult
nsCameraControl::GetHelper(JSContext* aCx, PRUint32 aKey, JS::Value* aValue)
{
  nsTArray<CameraRegion> regionArray;

  mCameraCore->GetParameter(aKey, regionArray);

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

/* readonly attribute nsICameraCapabilities capabilities; */
NS_IMETHODIMP
nsCameraControl::GetCapabilities(nsICameraCapabilities** aCapabilities)
{
  if (!mCapabilities) {
    mCapabilities = new nsCameraCapabilities(this);
  }

  nsCOMPtr<nsICameraCapabilities> capabilities = mCapabilities;
  capabilities.forget(aCapabilities);
  return NS_OK;
}

/* attribute DOMString effect; */
NS_IMETHODIMP
nsCameraControl::GetEffect(nsAString& aEffect)
{
  return GetHelper(CAMERA_PARAM_EFFECT, aEffect);
}
NS_IMETHODIMP
nsCameraControl::SetEffect(const nsAString& aEffect)
{
  return SetHelper(CAMERA_PARAM_EFFECT, aEffect);
}

/* attribute DOMString whiteBalanceMode; */
NS_IMETHODIMP
nsCameraControl::GetWhiteBalanceMode(nsAString& aWhiteBalanceMode)
{
  return GetHelper(CAMERA_PARAM_WHITEBALANCE, aWhiteBalanceMode);
}
NS_IMETHODIMP
nsCameraControl::SetWhiteBalanceMode(const nsAString& aWhiteBalanceMode)
{
  return SetHelper(CAMERA_PARAM_WHITEBALANCE, aWhiteBalanceMode);
}

/* attribute DOMString sceneMode; */
NS_IMETHODIMP
nsCameraControl::GetSceneMode(nsAString& aSceneMode)
{
  return GetHelper(CAMERA_PARAM_SCENEMODE, aSceneMode);
}
NS_IMETHODIMP
nsCameraControl::SetSceneMode(const nsAString& aSceneMode)
{
  return SetHelper(CAMERA_PARAM_SCENEMODE, aSceneMode);
}

/* attribute DOMString flashMode; */
NS_IMETHODIMP
nsCameraControl::GetFlashMode(nsAString& aFlashMode)
{
  return GetHelper(CAMERA_PARAM_FLASHMODE, aFlashMode);
}
NS_IMETHODIMP
nsCameraControl::SetFlashMode(const nsAString& aFlashMode)
{
  return SetHelper(CAMERA_PARAM_FLASHMODE, aFlashMode);
}

/* attribute DOMString focusMode; */
NS_IMETHODIMP
nsCameraControl::GetFocusMode(nsAString& aFocusMode)
{
  return GetHelper(CAMERA_PARAM_FOCUSMODE, aFocusMode);
}
NS_IMETHODIMP
nsCameraControl::SetFocusMode(const nsAString& aFocusMode)
{
  return SetHelper(CAMERA_PARAM_FOCUSMODE, aFocusMode);
}

/* attribute double zoom; */
NS_IMETHODIMP
nsCameraControl::GetZoom(double* aZoom)
{
  return GetHelper(CAMERA_PARAM_ZOOM, aZoom);
}
NS_IMETHODIMP
nsCameraControl::SetZoom(double aZoom)
{
  return SetHelper(CAMERA_PARAM_ZOOM, aZoom);
}

/* attribute jsval meteringAreas; */
NS_IMETHODIMP
nsCameraControl::GetMeteringAreas(JSContext* cx, JS::Value* aMeteringAreas)
{
  return GetHelper(cx, CAMERA_PARAM_METERINGAREAS, aMeteringAreas);
}
NS_IMETHODIMP
nsCameraControl::SetMeteringAreas(JSContext* cx, const JS::Value& aMeteringAreas)
{
  return SetHelper(cx, CAMERA_PARAM_METERINGAREAS, aMeteringAreas, mMaxMeteringAreas);
}

/* attribute jsval focusAreas; */
NS_IMETHODIMP
nsCameraControl::GetFocusAreas(JSContext* cx, JS::Value* aFocusAreas)
{
  return GetHelper(cx, CAMERA_PARAM_FOCUSAREAS, aFocusAreas);
}
NS_IMETHODIMP
nsCameraControl::SetFocusAreas(JSContext* cx, const JS::Value& aFocusAreas)
{
  return SetHelper(cx, CAMERA_PARAM_FOCUSAREAS, aFocusAreas, mMaxFocusAreas);
}

/* readonly attribute double focalLength; */
NS_IMETHODIMP
nsCameraControl::GetFocalLength(double* aFocalLength)
{
  return GetHelper(CAMERA_PARAM_FOCALLENGTH, aFocalLength);
}

/* readonly attribute double focusDistanceNear; */
NS_IMETHODIMP
nsCameraControl::GetFocusDistanceNear(double* aFocusDistanceNear)
{
  return GetHelper(CAMERA_PARAM_FOCUSDISTANCENEAR, aFocusDistanceNear);
}

/* readonly attribute double focusDistanceOptimum; */
NS_IMETHODIMP
nsCameraControl::GetFocusDistanceOptimum(double* aFocusDistanceOptimum)
{
  return GetHelper(CAMERA_PARAM_FOCUSDISTANCEOPTIMUM, aFocusDistanceOptimum);
}

/* readonly attribute double focusDistanceFar; */
NS_IMETHODIMP
nsCameraControl::GetFocusDistanceFar(double* aFocusDistanceFar)
{
  return GetHelper(CAMERA_PARAM_FOCUSDISTANCEFAR, aFocusDistanceFar);
}

/* void setExposureCompensation (const JS::Value& aCompensation, JSContext* cx); */
NS_IMETHODIMP
nsCameraControl::SetExposureCompensation(const JS::Value& aCompensation, JSContext* cx)
{
  if (aCompensation.isNullOrUndefined()) {
    // use NaN to switch the camera back into auto mode
    return SetHelper(CAMERA_PARAM_EXPOSURECOMPENSATION, NAN);
  }

  double compensation;
  if (!JS_ValueToNumber(cx, aCompensation, &compensation)) {
    return NS_ERROR_INVALID_ARG;
  }

  return SetHelper(CAMERA_PARAM_EXPOSURECOMPENSATION, compensation);
}

/* readonly attribute double exposureCompensation; */
NS_IMETHODIMP
nsCameraControl::GetExposureCompensation(double* aExposureCompensation)
{
  return GetHelper(CAMERA_PARAM_EXPOSURECOMPENSATION, aExposureCompensation);
}

/* attribute nsICameraShutterCallback onShutter; */
NS_IMETHODIMP
nsCameraControl::GetOnShutter(nsICameraShutterCallback** aOnShutter)
{
  *aOnShutter = mOnShutterCb;
  return NS_OK;
}
NS_IMETHODIMP
nsCameraControl::SetOnShutter(nsICameraShutterCallback* aOnShutter)
{
  mOnShutterCb = aOnShutter;
  return NS_OK;
}

/* void startRecording (in jsval aOptions, in nsICameraStartRecordingCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP
nsCameraControl::StartRecording(const JS::Value& aOptions, nsICameraStartRecordingCallback* onSuccess, nsICameraErrorCallback* onError, JSContext* cx)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  CameraSize size;
  nsresult rv = size.Init(cx, &aOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  return mCameraCore->StartRecording(size, onSuccess, onError);
}

/* void stopRecording (); */
NS_IMETHODIMP
nsCameraControl::StopRecording()
{
  return mCameraCore->StopRecording();
}

/* [implicit_jscontext] void getPreviewStream (in jsval aOptions, in nsICameraPreviewStreamCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP
nsCameraControl::GetPreviewStream(const JS::Value& aOptions, nsICameraPreviewStreamCallback* onSuccess, nsICameraErrorCallback* onError, JSContext* cx)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  CameraSize size;
  nsresult rv = size.Init(cx, &aOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  return mCameraCore->GetPreviewStream(size, onSuccess, onError);
}

/* void autoFocus (in nsICameraAutoFocusCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP
nsCameraControl::AutoFocus(nsICameraAutoFocusCallback* onSuccess, nsICameraErrorCallback* onError)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);
  return mCameraCore->AutoFocus(onSuccess, onError);
}

/* void takePicture (in jsval aOptions, in nsICameraTakePictureCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP nsCameraControl::TakePicture(const JS::Value& aOptions, nsICameraTakePictureCallback* onSuccess, nsICameraErrorCallback* onError, JSContext* cx)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  CameraPictureOptions  options;
  CameraSize            size;
  CameraPosition        pos;

  nsresult rv = options.Init(cx, &aOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = size.Init(cx, &options.pictureSize);
  NS_ENSURE_SUCCESS(rv, rv);

  /**
   * Default values, until the dictionary parser can handle them.
   * NaN indicates no value provided.
   */
  pos.latitude = NAN;
  pos.longitude = NAN;
  pos.altitude = NAN;
  pos.timestamp = NAN;
  rv = pos.Init(cx, &options.position);
  NS_ENSURE_SUCCESS(rv, rv);

  return mCameraCore->TakePicture(size, options.rotation, options.fileFormat, pos, onSuccess, onError);
}

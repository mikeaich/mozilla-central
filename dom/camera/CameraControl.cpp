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


DOMCI_DATA(CameraControl, nsICameraControl)

NS_INTERFACE_MAP_BEGIN(nsCameraControl)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsICameraControl)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CameraControl)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsCameraControl)
NS_IMPL_RELEASE(nsCameraControl)

/*
  Helper for string properties.
*/
static nsresult
setHelper(nsCameraControl *aCameraContol, PRUint32 aKey, const nsAString& aValue)
{
  const char *v = ToNewCString(aValue);
  if (v) {
    aCameraContol->SetParameter(aKey, v);
    nsMemory::Free(const_cast<char*>(v));
    return NS_OK;
  } else {
    return NS_ERROR_OUT_OF_MEMORY;
  }
}

/*
  Helper for doubles.
*/
static nsresult
setHelper(nsCameraControl *aCameraContol, PRUint32 aKey, double aValue)
{
  aCameraContol->SetParameter(aKey, aValue);
  return NS_OK;
}

/*
  Helper for weighted regions.
*/
static nsresult
setHelper(nsCameraControl *aCameraContol, PRUint32 aKey, const JS::Value & aValue, JSContext *cx)
{
  nsCameraControl::CameraRegion *parsedRegions;
  PRUint32 length = 0;

  if (aValue.isObject()) {
    JSObject *regions = JSVAL_TO_OBJECT(aValue);
    if (JS_IsArrayObject(cx, regions)) {
      if (JS_GetArrayLength(cx, regions, &length)) {
        DOM_CAMERA_LOGI("%s:%d : got %d regions\n", __func__, __LINE__, length);
        parsedRegions = new nsCameraControl::CameraRegion[length];
        for (PRUint32 i = 0; i < length; ++i) {
          jsval v;
          if (JS_GetElement(cx, regions, i, &v)) {
            if (v.isObject()) {
              nsCameraControl::CameraRegion* parsed = &parsedRegions[i];
              JSObject *r = JSVAL_TO_OBJECT(v);
              jsval p;

              /* TODO: move these Gonk-specific values somewhere else */
              PRInt32 top     = -1000;
              PRInt32 left    = -1000;
              PRInt32 bottom  =  1000;
              PRInt32 right   =  1000;
              PRUint32 weight =  1;

              if (JS_GetProperty(cx, r, "top", &p)) {
                if (JSVAL_IS_INT(p)) {
                  top = JSVAL_TO_INT(p);
                }
              }
              if (JS_GetProperty(cx, r, "left", &p)) {
                if (JSVAL_IS_INT(p)) {
                  left = JSVAL_TO_INT(p);
                }
              }
              if (JS_GetProperty(cx, r, "bottom", &p)) {
                if (JSVAL_IS_INT(p)) {
                  bottom = JSVAL_TO_INT(p);
                }
              }
              if (JS_GetProperty(cx, r, "right", &p)) {
                if (JSVAL_IS_INT(p)) {
                  right = JSVAL_TO_INT(p);
                }
              }
              if (JS_GetProperty(cx, r, "weight", &p)) {
                if (JSVAL_IS_INT(p)) {
                  weight = JSVAL_TO_INT(p);
                }
              }
              DOM_CAMERA_LOGI("region %d: top=%d, left=%d, bottom=%d, right=%d, weight=%d\n",
                i,
                top,
                left,
                bottom,
                right,
                weight
              );
              parsed->mTop = top;
              parsed->mLeft = left;
              parsed->mBottom = bottom;
              parsed->mRight = right;
              parsed->mWeight = weight;
            }
          }
        }
      }
    }
  }

  aCameraContol->SetParameter(aKey, parsedRegions, length);
  return NS_OK;
}

/* readonly attribute nsICameraCapabilities capabilities; */
NS_IMETHODIMP
nsCameraControl::GetCapabilities(nsICameraCapabilities * *aCapabilities)
{
  nsCOMPtr<nsICameraCapabilities> capabilities = mCapabilities;
  
  if (!capabilities) {
    capabilities = new nsCameraCapabilities(this);
    if (!capabilities) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  capabilities.forget(aCapabilities);
  return NS_OK;
}

/* attribute DOMString effect; */
NS_IMETHODIMP nsCameraControl::GetEffect(nsAString & aEffect)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsCameraControl::SetEffect(const nsAString & aEffect)
{
  return setHelper(this, CAMERA_PARAM_EFFECT, aEffect);
}

/* attribute DOMString whiteBalanceMode; */
NS_IMETHODIMP nsCameraControl::GetWhiteBalanceMode(nsAString & aWhiteBalanceMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsCameraControl::SetWhiteBalanceMode(const nsAString & aWhiteBalanceMode)
{
  return setHelper(this, CAMERA_PARAM_WHITEBALANCE, aWhiteBalanceMode);
}

/* attribute DOMString sceneMode; */
NS_IMETHODIMP nsCameraControl::GetSceneMode(nsAString & aSceneMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsCameraControl::SetSceneMode(const nsAString & aSceneMode)
{
  return setHelper(this, CAMERA_PARAM_SCENEMODE, aSceneMode);
}

/* attribute DOMString flashMode; */
NS_IMETHODIMP nsCameraControl::GetFlashMode(nsAString & aFlashMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsCameraControl::SetFlashMode(const nsAString & aFlashMode)
{
  return setHelper(this, CAMERA_PARAM_FLASHMODE, aFlashMode);
}

/* attribute DOMString focusMode; */
NS_IMETHODIMP nsCameraControl::GetFocusMode(nsAString & aFocusMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsCameraControl::SetFocusMode(const nsAString & aFocusMode)
{
  return setHelper(this, CAMERA_PARAM_FOCUSMODE, aFocusMode);
}

/* attribute double zoom; */
NS_IMETHODIMP nsCameraControl::GetZoom(double *aZoom)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsCameraControl::SetZoom(double aZoom)
{
  return setHelper(this, CAMERA_PARAM_ZOOM, aZoom);
}

/* attribute jsval meteringAreas; */
NS_IMETHODIMP nsCameraControl::GetMeteringAreas(JSContext *cx, JS::Value *aMeteringAreas)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsCameraControl::SetMeteringAreas(JSContext *cx, const JS::Value & aMeteringAreas)
{
  return setHelper(this, CAMERA_PARAM_METERINGAREAS, aMeteringAreas, cx);
}

/* attribute jsval focusAreas; */
NS_IMETHODIMP nsCameraControl::GetFocusAreas(JSContext *cx, JS::Value *aFocusAreas)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsCameraControl::SetFocusAreas(JSContext *cx, const JS::Value & aFocusAreas)
{
  return setHelper(this, CAMERA_PARAM_FOCUSAREAS, aFocusAreas, cx);
}

/* readonly attribute double focalLength; */
NS_IMETHODIMP nsCameraControl::GetFocalLength(double *aFocalLength)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute double focusDistanceNear; */
NS_IMETHODIMP nsCameraControl::GetFocusDistanceNear(double *aFocusDistanceNear)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute double focusDistanceOptimum; */
NS_IMETHODIMP nsCameraControl::GetFocusDistanceOptimum(double *aFocusDistanceOptimum)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute double focusDistanceFar; */
NS_IMETHODIMP nsCameraControl::GetFocusDistanceFar(double *aFocusDistanceFar)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setExposureCompensation ([optional] in double compensation); */
NS_IMETHODIMP
nsCameraControl::SetExposureCompensation(double compensation)
{
  return setHelper(this, CAMERA_PARAM_WHITEBALANCE, compensation);
}

/* readonly attribute double exposureCompensation; */
NS_IMETHODIMP nsCameraControl::GetExposureCompensation(double *aExposureCompensation)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsICameraShutterCallback onShutter; */
NS_IMETHODIMP nsCameraControl::GetOnShutter(nsICameraShutterCallback * *aOnShutter)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsCameraControl::SetOnShutter(nsICameraShutterCallback *aOnShutter)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void startRecording (in jsval aOptions, in nsICameraStartRecordingCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP
nsCameraControl::StartRecording(const JS::Value & aOptions, nsICameraStartRecordingCallback *onSuccess, nsICameraErrorCallback *onError, JSContext* cx)
{
  /* 0 means not specified, use default value */
  PRUint32 width = 0;
  PRUint32 height = 0;

  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  if (aOptions.isObject()) {
    JSObject *options = JSVAL_TO_OBJECT(aOptions);
    jsval v;

    if (JS_GetProperty(cx, options, "width", &v)) {
      if (JSVAL_IS_INT(v)) {
        width = JSVAL_TO_INT(v);
      }
    }
    if (JS_GetProperty(cx, options, "height", &v)) {
      if (JSVAL_IS_INT(v)) {
        height = JSVAL_TO_INT(v);
      }
    }
  }

  nsCOMPtr<nsIRunnable> startRecordingTask = new StartRecordingTask(this, width, height, onSuccess, onError);
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
  /* 0 means not specified, use default value */
  PRUint32 width = 0;
  PRUint32 height = 0;

  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  if (aOptions.isObject()) {
    JSObject *options = JSVAL_TO_OBJECT(aOptions);
    jsval v;

    if (JS_GetProperty(cx, options, "width", &v)) {
      if (JSVAL_IS_INT(v)) {
        width = JSVAL_TO_INT(v);
      }
    }
    if (JS_GetProperty(cx, options, "height", &v)) {
      if (JSVAL_IS_INT(v)) {
        height = JSVAL_TO_INT(v);
      }
    }
  }

  nsCOMPtr<nsIRunnable> getPreviewStreamTask = new GetPreviewStreamTask(this, width, height, onSuccess, onError);
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
  PRUint32 width = 0;
  PRUint32 height = 0;
  PRInt32 rotation = 0;
  double latitude = 0;
  double longitude = 0;
  double altitude = 0;
  double timestamp = 0;
  bool latitudeSet = false;
  bool longitudeSet = false;
  bool altitudeSet = false;
  bool timestampSet = false;

  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);
  NS_ENSURE_TRUE(aOptions, NS_ERROR_INVALID_ARG);

  jsval pictureSize;
  nsresult rv = aOptions->GetPictureSize(&pictureSize);
  NS_ENSURE_SUCCESS(rv, rv);
  if (pictureSize.isObject()) {
    JSObject* options = JSVAL_TO_OBJECT(pictureSize);
    jsval v;
    
    if (JS_GetProperty(cx, options, "width", &v)) {
      if (JSVAL_IS_INT(v)) {
        width = JSVAL_TO_INT(v);
      }
    }
    if (JS_GetProperty(cx, options, "height", &v)) {
      if (JSVAL_IS_INT(v)) {
        height = JSVAL_TO_INT(v);
      }
    }
  }
  
  nsString fileFormat;
  rv = aOptions->GetFileFormat(fileFormat);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = aOptions->GetRotation(&rotation);
  NS_ENSURE_SUCCESS(rv, rv);
  
  jsval position;
  rv = aOptions->GetPosition(&position);
  NS_ENSURE_SUCCESS(rv, rv);
  if (position.isObject()) {
    JSObject* options = JSVAL_TO_OBJECT(position);
    jsval v;
    
    if (JS_GetProperty(cx, options, "latitude", &v)) {
      if (JSVAL_IS_NUMBER(v)) {
        if (JS_ValueToNumber(cx, v, &latitude)) {
          latitudeSet = true;
        }
      }
    }
    if (JS_GetProperty(cx, options, "longitude", &v)) {
      if (JSVAL_IS_NUMBER(v)) {
        if (JS_ValueToNumber(cx, v, &longitude)) {
          longitudeSet = true;
        }
      }
    }
    if (JS_GetProperty(cx, options, "altitude", &v)) {
      if (JSVAL_IS_NUMBER(v)) {
        if (JS_ValueToNumber(cx, v, &altitude)) {
          altitudeSet = true;
        }
      }
    }
    if (JS_GetProperty(cx, options, "timestamp", &v)) {
      if (JSVAL_IS_NUMBER(v)) {
        if (JS_ValueToNumber(cx, v, &timestamp)) {
          timestampSet = true;
        }
      }
    }
  }

  nsCOMPtr<nsIRunnable> takePictureTask = new TakePictureTask(this, width, height, rotation, fileFormat, latitude, latitudeSet, longitude, longitudeSet, altitude, altitudeSet, timestamp, timestampSet, onSuccess, onError);
  mCameraThread->Dispatch(takePictureTask, NS_DISPATCH_NORMAL);

  return NS_OK;
}

void
nsCameraControl::ReceiveFrame(PRUint8* aData, PRUint32 aLength)
{
  if (mPreview) {
    mPreview->ReceiveFrame(aData, aLength);
  }
}

void
nsCameraControl::AutoFocusComplete(bool aSuccess)
{
  nsCOMPtr<nsIRunnable> autoFocusResult = new AutoFocusResult(aSuccess, mAutoFocusOnSuccessCb);

  if (NS_FAILED(NS_DispatchToMainThread(autoFocusResult))) {
    NS_WARNING("Failed to dispatch autoFocus() onSuccess callback to main thread!");
  }
}

void
nsCameraControl::TakePictureComplete(PRUint8* aData, PRUint32 aLength)
{
  PRUint8* data = new PRUint8[aLength];

  memcpy(data, aData, aLength);

  /* TODO: pick up the actual specified picture format for the MIME type;
     for now, assume we'll be using JPEGs. */
  nsIDOMBlob *blob = new nsDOMMemoryFile((void*)data, (PRUint64)aLength, NS_LITERAL_STRING("image/jpeg"));
  nsCOMPtr<nsIRunnable> takePictureResult = new TakePictureResult(blob, mTakePictureOnSuccessCb);

  if (NS_FAILED(NS_DispatchToMainThread(takePictureResult))) {
    NS_WARNING("Failed to dispatch takePicture() onSuccess callback to main thread!");
  }
}

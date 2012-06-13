/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "libcameraservice/CameraHardwareInterface.h"
#include "jsapi.h"
#include "DOMCameraManager.h"
#include "CameraControl.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"


NS_IMPL_ISUPPORTS1(nsCameraControl, nsICameraControl)

nsCameraControl::nsCameraControl()
{
  /* member initializers and constructor code */
}

nsCameraControl::~nsCameraControl()
{
  /* destructor code */
}

/* readonly attribute nsICameraCapabilities capabilities; */
NS_IMETHODIMP nsCameraControl::GetCapabilities(nsICameraCapabilities * *aCapabilities)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString effect; */
NS_IMETHODIMP nsCameraControl::GetEffect(nsAString & aEffect)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsCameraControl::SetEffect(const nsAString & aEffect)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString whiteBalanceMode; */
NS_IMETHODIMP nsCameraControl::GetWhiteBalanceMode(nsAString & aWhiteBalanceMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsCameraControl::SetWhiteBalanceMode(const nsAString & aWhiteBalanceMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString sceneMode; */
NS_IMETHODIMP nsCameraControl::GetSceneMode(nsAString & aSceneMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsCameraControl::SetSceneMode(const nsAString & aSceneMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString flashMode; */
NS_IMETHODIMP nsCameraControl::GetFlashMode(nsAString & aFlashMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsCameraControl::SetFlashMode(const nsAString & aFlashMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString focusMode; */
NS_IMETHODIMP nsCameraControl::GetFocusMode(nsAString & aFocusMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsCameraControl::SetFocusMode(const nsAString & aFocusMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute double zoom; */
NS_IMETHODIMP nsCameraControl::GetZoom(double *aZoom)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsCameraControl::SetZoom(double aZoom)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute jsval meteringAreas; */
NS_IMETHODIMP nsCameraControl::GetMeteringAreas(JS::Value *aMeteringAreas)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsCameraControl::SetMeteringAreas(const JS::Value & aMeteringAreas)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute jsval focusAreas; */
NS_IMETHODIMP nsCameraControl::GetFocusAreas(JS::Value *aFocusAreas)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsCameraControl::SetFocusAreas(const JS::Value & aFocusAreas)
{
  return NS_ERROR_NOT_IMPLEMENTED;
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
NS_IMETHODIMP nsCameraControl::SetExposureCompensation(double compensation)
{
  return NS_ERROR_NOT_IMPLEMENTED;
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

/* void autoFocus (in nsICameraAutoFocusCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP nsCameraControl::AutoFocus(nsICameraAutoFocusCallback *onSuccess, nsICameraErrorCallback *onError)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void takePicture (in nsICameraTakePictureCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP nsCameraControl::TakePicture(nsICameraTakePictureCallback *onSuccess, nsICameraErrorCallback *onError)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void startRecording (in jsval aOptions, in nsICameraStartRecordingCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP nsCameraControl::StartRecording(const JS::Value & aOptions, nsICameraStartRecordingCallback *onSuccess, nsICameraErrorCallback *onError)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void stopRecording (); */
NS_IMETHODIMP nsCameraControl::StopRecording()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* [implicit_jscontext] void getPreviewStream (in jsval aOptions, in nsICameraPreviewStreamCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP nsCameraControl::GetPreviewStream(const JS::Value & aOptions, nsICameraPreviewStreamCallback *onSuccess, nsICameraErrorCallback *onError, JSContext* cx)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* [implicit_jscontext] void getCamera ([optional] in jsval aOptions, in nsICameraGetCameraCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP
nsDOMCameraManager::GetCamera(const JS::Value & aOptions, nsICameraGetCameraCallback *onSuccess, nsICameraErrorCallback *onError, JSContext* cx)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* [implicit_jscontext] jsval getListOfCameras (); */
NS_IMETHODIMP
nsDOMCameraManager::GetListOfCameras(JSContext* cx, JS::Value *_retval NS_OUTPARAM)
{
  JSObject* a = JS_NewArrayObject(cx, 0, nsnull);
  camera_module_t* module;
  PRUint32 index = 0;
  PRUint32 count;
  
  if (!a) {
    DOM_CAMERA_LOGE("getListOfCameras : Could not create array object");
    return NS_ERROR_OUT_OF_MEMORY;
  }
  if (hw_get_module(CAMERA_HARDWARE_MODULE_ID,
            (const hw_module_t **)&module) < 0) {
    DOM_CAMERA_LOGE("getListOfCameras : Could not load camera HAL module");
    return NS_ERROR_NOT_AVAILABLE;
  }

  count = module->get_number_of_cameras();
  DOM_CAMERA_LOGI("getListOfCameras : get_number_of_cameras() returned %d\n", count);
  while (count--) {
    JSString* v;

    switch (count) {
      case 0:
        v = JS_NewStringCopyZ(cx, "back");
        break;
      
      case 1:
        v = JS_NewStringCopyZ(cx, "front");
        break;
      
      default:
        // TODO: add a unique identifier for each one...
        v = JS_NewStringCopyZ(cx, "extra-camera");
        break;
    }
    if (!v) {
      DOM_CAMERA_LOGE("getListOfCameras : out of memory populating camera list");
      // TODO: clean up any partial objects?
      return NS_ERROR_NOT_AVAILABLE;
    }
    JS_SetElement(cx, a, index++, &STRING_TO_JSVAL(v));
  }

  *_retval = OBJECT_TO_JSVAL(a);
  return NS_OK;
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string.h>
#include "libcameraservice/CameraHardwareInterface.h"
#include "camera/CameraParameters.h"
#include "nsCOMPtr.h"
#include "nsDOMClassInfo.h"
#include "jsapi.h"
#include "nsThread.h"
#include "DOMCameraManager.h"
#include "CameraControl.h"
#include "GonkCameraHwMgr.h"
#include "CameraCapabilities.h"
#include "GonkCameraControl.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"


// NS_IMPL_ISUPPORTS1(nsCameraControl, nsICameraControl)

DOMCI_DATA(CameraControl, nsICameraControl)

NS_INTERFACE_MAP_BEGIN(nsCameraControl)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsICameraControl)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CameraControl)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsCameraControl)
NS_IMPL_RELEASE(nsCameraControl)


nsCameraControl::nsCameraControl(PRUint32 aCameraId, nsIThread *aCameraThread)
  : mCameraId(aCameraId)
  , mCameraThread(aCameraThread)
  , mCapabilities(nsnull)
  , mHwHandle(0)
{
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
  mHwHandle = GonkCameraHardware::getCameraHardwareHandle(this, mCameraId);
  DOM_CAMERA_LOGI("%s:%d : this = %p, mHwHandle = %d\n", __func__, __LINE__, this, mHwHandle);
}

nsCameraControl::~nsCameraControl()
{
  DOM_CAMERA_LOGI("%s:%d : this = %p, mHwHandle = %d\n", __func__, __LINE__, this, mHwHandle);
  GonkCameraHardware::releaseCameraHardwareHandle(mHwHandle);
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
}

/* readonly attribute nsICameraCapabilities capabilities; */
NS_IMETHODIMP nsCameraControl::GetCapabilities(nsICameraCapabilities * *aCapabilities)
{
  nsRefPtr<nsICameraCapabilities> capabilities = mCapabilities;
  
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

  nsCOMPtr<nsIRunnable> doGetPreviewStream = new DoGetPreviewStream(this, width, height, onSuccess, onError);
  mCameraThread->Dispatch(doGetPreviewStream, NS_DISPATCH_NORMAL);

  return NS_OK;
}

const char*
nsCameraControl::GetParameter(const char* key)
{
  return GonkCameraHardware::getCameraHardwareParameter(mHwHandle, key);
}

void
nsCameraControl::SetParameter(const char* key, const char* value)
{
  GonkCameraHardware::setCameraHardwareParameter(mHwHandle, key, value);
}

void
nsCameraControl::ReceiveImage(PRUint8* aData, PRUint32 aLength)
{
}

void
nsCameraControl::AutoFocusComplete(bool success)
{
}

void
nsCameraControl::ReceiveFrame(PRUint8* aData, PRUint32 aLength)
{
  if (mPreview) {
    mPreview->ReceiveFrame(aData, aLength);
  }
}

void
GonkCameraReceiveImage(nsCameraControl* gc, PRUint8* aData, PRUint32 aLength)
{
  gc->ReceiveImage(aData, aLength);
}

void
GonkCameraAutoFocusComplete(nsCameraControl* gc, bool success)
{
  gc->AutoFocusComplete(success);
}

void
GonkCameraReceiveFrame(nsCameraControl* gc, PRUint8* aData, PRUint32 aLength)
{
  gc->ReceiveFrame(aData, aLength);
}

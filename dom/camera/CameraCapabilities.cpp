/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string.h>
#include "jsapi.h"
#include "camera/CameraParameters.h"
#include "CameraControl.h"
#include "CameraCapabilities.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"


using namespace android;

#define CHECK_CAMERA_PTR(c)         \
  if (!c) {                         \
    return NS_ERROR_NOT_AVAILABLE;  \
  }

NS_IMPL_ISUPPORTS1(nsCameraCapabilities, nsICameraCapabilities)

nsCameraCapabilities::nsCameraCapabilities(nsRefPtr<nsCameraControl> aCamera) :
  mCamera(aCamera)
{
  /* member initializers and constructor code */
}

nsCameraCapabilities::~nsCameraCapabilities()
{
  /* destructor code */
  mCamera = nsnull;
}

static nsresult
parseZoomRatioItemAndAdd(JSContext *cx, JSObject *array, PRUint32 index, const char *start, char **end)
{
  static const double ZOOM_RATIO_SCALING_FACTOR = 100;
  double d = strtod(start, end);
  jsval v;

  d /= ZOOM_RATIO_SCALING_FACTOR;
  if (!JS_NewNumberValue(cx, d, &v)) {
    return NS_ERROR_FAILURE;
  }
  if (!JS_SetElement(cx, array, index, &v)) {
    return NS_ERROR_FAILURE;
  }
  
  return NS_OK;
}

nsresult
nsCameraCapabilities::parameterListToNewArray(JSContext *cx, JSObject **array, const char *key, nsresult (*parseItemAndAdd)(JSContext *cx, JSObject *array, PRUint32 index, const char *start,  char **end))
{
  CHECK_CAMERA_PTR(mCamera);

  PRUint32 index = 0;
  const char *value;
  nsresult rv;
  
  value = mCamera->GetParameter(key);
  if (!value) {
    /* in case we get nonsense data back */
    *array = nsnull;
    return NS_OK;
  }
  
  *array = JS_NewArrayObject(cx, 0, nsnull);
  
  const char* p = value;
  char* q;
  while (p) {
    q = strchr(p, ',');
    if (q != p) { /* skip consecutive delimiters, just in case */
      rv = parseItemAndAdd(cx, *array, index, p, &q);
      if (rv != NS_OK) {
        return rv;
      }
      index += 1;
    }
    p = q;
    if (p) {
      p += 1;
    }
  }

  return NS_OK;
}

/* readonly attribute jsval previewSizes; */
NS_IMETHODIMP
nsCameraCapabilities::GetPreviewSizes(JSContext* cx, JS::Value *aPreviewSizes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval pictureSizes; */
NS_IMETHODIMP
nsCameraCapabilities::GetPictureSizes(JSContext* cx, JS::Value *aPictureSizes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval fileFormats; */
NS_IMETHODIMP
nsCameraCapabilities::GetFileFormats(JSContext* cx, JS::Value *aFileFormats)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval whiteBalanceModes; */
NS_IMETHODIMP
nsCameraCapabilities::GetWhiteBalanceModes(JSContext* cx, JS::Value *aWhiteBalanceModes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  PRUint32 index = 0;
  const char* value;
  JSObject* rv;

  value = mCamera->GetParameter(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE);
  if (!value) {
    /* in case we get nonsense data back */
    *aWhiteBalanceModes = JSVAL_NULL;
    return NS_OK;
  }
  
  rv = JS_NewArrayObject(cx, 0, nsnull);
  
  const char* p = value;
  const char* q;
  while ((q = strchr(p, ',')) != nsnull) {
    if (q != p) { /* skip consecutive delimiters, just in case */
      JSString* v = JS_NewStringCopyN(cx, p, q - p);
      if (!v) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      if (!JS_SetElement(cx, rv, index++, &STRING_TO_JSVAL(v))) {
        return NS_ERROR_FAILURE;
      }
    }
    p = q + 1;
  }

  *aWhiteBalanceModes = OBJECT_TO_JSVAL(rv);
  return NS_OK;
}

/* readonly attribute jsval sceneModes; */
NS_IMETHODIMP
nsCameraCapabilities::GetSceneModes(JSContext* cx, JS::Value *aSceneModes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval effects; */
NS_IMETHODIMP
nsCameraCapabilities::GetEffects(JSContext* cx, JS::Value *aEffects)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval flashModes; */
NS_IMETHODIMP
nsCameraCapabilities::GetFlashModes(JSContext* cx, JS::Value *aFlashModes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval focusModes; */
NS_IMETHODIMP
nsCameraCapabilities::GetFocusModes(JSContext* cx, JS::Value *aFocusModes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long maxFocusAreas; */
NS_IMETHODIMP
nsCameraCapabilities::GetMaxFocusAreas(JSContext* cx, PRInt32 *aMaxFocusAreas)
{
  CHECK_CAMERA_PTR(mCamera);
  
  const char* value = mCamera->GetParameter(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS);
  if (!value) {
    /* in case we get nonsense data back */
    *aMaxFocusAreas = 0;
    return NS_OK;
  }
  
  *aMaxFocusAreas = atoi(value);
  return NS_OK;
}

/* readonly attribute double minExposureCompensation; */
NS_IMETHODIMP
nsCameraCapabilities::GetMinExposureCompensation(JSContext* cx, double *aMinExposureCompensation)
{
  CHECK_CAMERA_PTR(mCamera);
  
  const char* value = mCamera->GetParameter(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION);
  if (!value) {
    /* in case we get nonsense data back */
    *aMinExposureCompensation = 0;
    return NS_OK;
  }
  
  *aMinExposureCompensation = atof(value);
  return NS_OK;
}

/* readonly attribute double maxExposureCompensation; */
NS_IMETHODIMP
nsCameraCapabilities::GetMaxExposureCompensation(JSContext* cx, double *aMaxExposureCompensation)
{
  CHECK_CAMERA_PTR(mCamera);
  
  const char* value = mCamera->GetParameter(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION);
  if (!value) {
    /* in case we get nonsense data back */
    *aMaxExposureCompensation = 0;
    return NS_OK;
  }
  
  *aMaxExposureCompensation = atof(value);
  return NS_OK;
}

/* readonly attribute double stepExposureCompensation; */
NS_IMETHODIMP
nsCameraCapabilities::GetStepExposureCompensation(JSContext* cx, double *aStepExposureCompensation)
{
  CHECK_CAMERA_PTR(mCamera);
  
  const char* value = mCamera->GetParameter(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP);
  if (!value) {
    /* in case we get nonsense data back */
    *aStepExposureCompensation = 0;
    return NS_OK;
  }
  
  *aStepExposureCompensation = atof(value);
  return NS_OK;
}

/* readonly attribute long maxMeteringAreas; */
NS_IMETHODIMP
nsCameraCapabilities::GetMaxMeteringAreas(JSContext* cx, PRInt32 *aMaxMeteringAreas)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval zoomRatios; */
NS_IMETHODIMP
nsCameraCapabilities::GetZoomRatios(JSContext* cx, JS::Value *aZoomRatios)
{
  CHECK_CAMERA_PTR(mCamera);
  
  const char* value;
  JSObject* array;
  nsresult rv;

  value = mCamera->GetParameter(CameraParameters::KEY_ZOOM_SUPPORTED);
  if (!value || strcmp(value, CameraParameters::TRUE) != 0) {
    /* if zoom is not supported, return a null object */
    *aZoomRatios = JSVAL_NULL;
    return NS_OK;
  }
  
  rv = parameterListToNewArray(cx, &array, CameraParameters::KEY_ZOOM_RATIOS, parseZoomRatioItemAndAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aZoomRatios = OBJECT_TO_JSVAL(array);
  return NS_OK;
}

/* readonly attribute jsval videoSizes; */
NS_IMETHODIMP
nsCameraCapabilities::GetVideoSizes(JSContext* cx, JS::Value *aVideoSizes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

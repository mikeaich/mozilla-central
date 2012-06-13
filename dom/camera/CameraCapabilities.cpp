/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CameraCapabilities.h"


NS_IMPL_ISUPPORTS1(nsCameraCapabilities, nsICameraCapabilities)

#define CHECK_CAMERA_PTR(c)         \
  if (!c) {                         \
    return NS_ERROR_NOT_AVAILABLE;  \
  }

nsCameraCapabilities::nsCameraCapabilities(nsCOMPtr<nsIDOMCameraManager> aCamera) :
  mCamera(aCamera)
{
  /* member initializers and constructor code */
}

nsCameraCapabilities::~nsCameraCapabilities()
{
  /* destructor code */
  mCamera = nsnull;
}

/* readonly attribute jsval previewSizes; */
NS_IMETHODIMP
nsCameraCapabilities::GetPreviewSizes(JS::Value *aPreviewSizes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval pictureSizes; */
NS_IMETHODIMP
nsCameraCapabilities::GetPictureSizes(JS::Value *aPictureSizes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval fileFormats; */
NS_IMETHODIMP
nsCameraCapabilities::GetFileFormats(JS::Value *aFileFormats)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval whiteBalanceModes; */
NS_IMETHODIMP
nsCameraCapabilities::GetWhiteBalanceModes(JS::Value *aWhiteBalanceModes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval sceneModes; */
NS_IMETHODIMP
nsCameraCapabilities::GetSceneModes(JS::Value *aSceneModes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval effects; */
NS_IMETHODIMP
nsCameraCapabilities::GetEffects(JS::Value *aEffects)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval flashModes; */
NS_IMETHODIMP
nsCameraCapabilities::GetFlashModes(JS::Value *aFlashModes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval focusModes; */
NS_IMETHODIMP
nsCameraCapabilities::GetFocusModes(JS::Value *aFocusModes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long maxFocusAreas; */
NS_IMETHODIMP
nsCameraCapabilities::GetMaxFocusAreas(PRInt32 *aMaxFocusAreas)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute double minExposureCompensation; */
NS_IMETHODIMP
nsCameraCapabilities::GetMinExposureCompensation(double *aMinExposureCompensation)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute double maxExposureCompensation; */
NS_IMETHODIMP
nsCameraCapabilities::GetMaxExposureCompensation(double *aMaxExposureCompensation)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute double stepExposureCompensation; */
NS_IMETHODIMP
nsCameraCapabilities::GetStepExposureCompensation(double *aStepExposureCompensation)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long maxMeteringAreas; */
NS_IMETHODIMP
nsCameraCapabilities::GetMaxMeteringAreas(PRInt32 *aMaxMeteringAreas)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval zoomRatios; */
NS_IMETHODIMP
nsCameraCapabilities::GetZoomRatios(JS::Value *aZoomRatios)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute jsval videoSizes; */
NS_IMETHODIMP
nsCameraCapabilities::GetVideoSizes(JS::Value *aVideoSizes)
{
  CHECK_CAMERA_PTR(mCamera);
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

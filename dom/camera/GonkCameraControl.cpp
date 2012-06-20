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


/*
  Gonk-specific CameraControl implementation.
*/

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

nsresult
nsCameraControl::DoGetPreviewStream(GetPreviewStreamTask *aGetPreviewStream)
{
  nsCOMPtr<CameraPreview> preview = mPreview;
  
  if (!preview) {
    preview = new CameraPreview(mHwHandle, aGetPreviewStream->mWidth, aGetPreviewStream->mHeight);
    if (!preview) {
      if (NS_FAILED(NS_DispatchToMainThread(new CameraErrorResult(aGetPreviewStream->mOnErrorCb, NS_LITERAL_STRING("OUT_OF_MEMORY"))))) {
        NS_WARNING("Failed to dispatch getPreviewStream() onError callback to main thread!");
      }
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  mPreview = preview;
  if (NS_FAILED(NS_DispatchToMainThread(new GetPreviewStreamResult(preview.get(), aGetPreviewStream->mOnSuccessCb)))) {
    NS_WARNING("Failed to dispatch getPreviewStream() onSuccess callback to main thread!");
  }
  return NS_OK;
}

nsresult
nsCameraControl::DoAutoFocus(AutoFocusTask *aAutoFocus)
{
  nsCOMPtr<nsICameraAutoFocusCallback> cb = mAutoFocusOnSuccessCb;
  if (cb) {
    /* we already have a callback, so someone has already called autoFocus() -- cancel it */
    mAutoFocusOnSuccessCb = nsnull;
    nsCOMPtr<nsICameraErrorCallback> ecb = mAutoFocusOnErrorCb;
    mAutoFocusOnErrorCb = nsnull;
    if (ecb) {
      if (NS_FAILED(NS_DispatchToMainThread(new CameraErrorResult(ecb, NS_LITERAL_STRING("CANCELLED"))))) {
        NS_WARNING("Failed to dispatch old autoFocus() onError callback to main thread!");
      }
    }

    GonkCameraHardware::doCameraHardwareCancelAutoFocus(mHwHandle);
  }

  mAutoFocusOnSuccessCb = aAutoFocus->mOnSuccessCb;
  mAutoFocusOnErrorCb = aAutoFocus->mOnErrorCb;

  if (GonkCameraHardware::doCameraHardwareAutoFocus(mHwHandle) == OK) {
    return NS_OK;
  } else {
    return NS_ERROR_FAILURE;
  }
}

nsresult
nsCameraControl::DoTakePicture(TakePictureTask *aTakePicture)
{
  nsCOMPtr<nsICameraTakePictureCallback> cb = mTakePictureOnSuccessCb;
  if (cb) {
    /* we already have a callback, so someone has already called TakePicture() -- cancel it */
    mTakePictureOnSuccessCb = nsnull;
    nsCOMPtr<nsICameraErrorCallback> ecb = mTakePictureOnErrorCb;
    mTakePictureOnErrorCb = nsnull;
    if (ecb) {
      if (NS_FAILED(NS_DispatchToMainThread(new CameraErrorResult(ecb, NS_LITERAL_STRING("CANCELLED"))))) {
        NS_WARNING("Failed to dispatch old TakePicture() onError callback to main thread!");
      }
    }

    GonkCameraHardware::doCameraHardwareCancelTakePicture(mHwHandle);
  }

  mTakePictureOnSuccessCb = aTakePicture->mOnSuccessCb;
  mTakePictureOnErrorCb = aTakePicture->mOnErrorCb;
  
  /* convert 'rotation' to a positive value from 0..270 degrees, in steps of 90 */
  PRUint32 r = static_cast<PRUint32>(aTakePicture->mRotation);
  r %= 360;
  r += 45;
  r /= 90;
  r *= 90;

  if (GonkCameraHardware::doCameraHardwareTakePicture(mHwHandle) == OK) {
    return NS_OK;
  } else {
    return NS_ERROR_FAILURE;
  }
}

nsresult
nsCameraControl::DoStartRecording(StartRecordingTask *aStartRecording)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsCameraControl::DoStopRecording(StopRecordingTask *aStopRecording)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

void
GonkCameraReceiveImage(nsCameraControl* gc, PRUint8* aData, PRUint32 aLength)
{
  gc->TakePictureComplete(aData, aLength);
}

void
GonkCameraAutoFocusComplete(nsCameraControl* gc, bool success)
{
  gc->AutoFocusComplete(success);
}

void
GonkCameraReceiveFrame(nsCameraControl* gc, PRUint8* aData, PRUint32 aLength)
{
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
  gc->ReceiveFrame(aData, aLength);
}


/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string.h>
#include "libcameraservice/CameraHardwareInterface.h"
#include "camera/CameraParameters.h"
#include "nsCOMPtr.h"
#include "nsDOMClassInfo.h"
#include "nsMemory.h"
#include "jsapi.h"
#include "nsThread.h"
#include "DOMCameraManager.h"
#include "CameraControl.h"
#include "GonkCameraHwMgr.h"
#include "CameraCapabilities.h"
#include "GonkCameraControl.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"


static const char* getKeyText(PRUint32 aKey)
{
  switch (aKey) {
    case nsCameraControl::CAMERA_PARAM_EFFECT:
      return CameraParameters::KEY_EFFECT;
    case nsCameraControl::CAMERA_PARAM_WHITEBALANCE:
      return CameraParameters::KEY_WHITE_BALANCE;
    case nsCameraControl::CAMERA_PARAM_SCENEMODE:
      return CameraParameters::KEY_SCENE_MODE;
    case nsCameraControl::CAMERA_PARAM_FLASHMODE:
      return CameraParameters::KEY_FLASH_MODE;
    case nsCameraControl::CAMERA_PARAM_FOCUSMODE:
      return CameraParameters::KEY_FOCUS_MODE;
    case nsCameraControl::CAMERA_PARAM_ZOOM:
      return CameraParameters::KEY_ZOOM;
    case nsCameraControl::CAMERA_PARAM_METERINGAREAS:
      return CameraParameters::KEY_METERING_AREAS;
    case nsCameraControl::CAMERA_PARAM_FOCUSAREAS:
      return CameraParameters::KEY_FOCUS_AREAS;
    case nsCameraControl::CAMERA_PARAM_FOCALLENGTH:
      return CameraParameters::KEY_FOCAL_LENGTH;
    case nsCameraControl::CAMERA_PARAM_FOCUSDISTANCENEAR:
      return CameraParameters::KEY_FOCUS_DISTANCES;
    case nsCameraControl::CAMERA_PARAM_FOCUSDISTANCEOPTIMUM:
      return CameraParameters::KEY_FOCUS_DISTANCES;
    case nsCameraControl::CAMERA_PARAM_FOCUSDISTANCEFAR:
      return CameraParameters::KEY_FOCUS_DISTANCES;
    case nsCameraControl::CAMERA_PARAM_EXPOSURECOMPENSATION:
      return CameraParameters::KEY_EXPOSURE_COMPENSATION;
    default:
      return nsnull;
  }
}

/*
  Gonk-specific CameraControl implementation.
*/

nsCameraControl::nsCameraControl(PRUint32 aCameraId, nsIThread *aCameraThread)
  : mCameraId(aCameraId)
  , mCameraThread(aCameraThread)
  , mCapabilities(nsnull)
  , mHwHandle(0)
  , mFileFormat(nsnull)
  , mDeferConfigUpdate(false)
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

  if (mFileFormat) {
    nsMemory::Free(const_cast<char*>(mFileFormat));
  }
}

const char*
nsCameraControl::GetParameter(const char *aKey)
{
  return mParams.get(aKey);
}

const char*
nsCameraControl::GetParameter(PRUint32 aKey)
{
  const char *key = getKeyText(aKey);
  if (key) {
    return mParams.get(key);
  } else {
    return nsnull;
  }
}

void
nsCameraControl::SetParameter(const char *aKey, const char *aValue)
{
  mParams.set(aKey, aValue);
}

void
nsCameraControl::SetParameter(PRUint32 aKey, const char *aValue)
{
  const char *key = getKeyText(aKey);
  if (key) {
    mParams.set(key, aValue);
  }
}

void
nsCameraControl::SetParameter(PRUint32 aKey, double aValue)
{
  const char *key = getKeyText(aKey);
  if (key) {
    mParams.setFloat(key, aValue);
  }
}

void
nsCameraControl::SetParameter(PRUint32 aKey, CameraRegion *aRegions, PRUint32 aLength)
{
  const char *key = getKeyText(aKey);
  if (key) {
    if (!aLength) {
      /* This tells the camera driver to revert to automatic regioning. */
      mParams.set(key, "(0,0,0,0,0)");
      return;
    }
    
    PRUint32 size = aLength * 31;
    char *s = new char[size];
    char *p = s;
    PRUint32 n;
    
    for (PRUint32 i = 0; i < aLength; ++i) {
      CameraRegion *r = &aRegions[i];
      n = snprintf(p, size, "(%d,%d,%d,%d,%d),", r->mTop, r->mLeft, r->mBottom, r->mRight, r->mWeight);
      if (n > size) {
        DOM_CAMERA_LOGE("needed %d bytes for region, but only have %d bytes\n", n, size);
        delete[] s;
        return;
      }
      size -= n;
      p += n;
    }

    *p = '\0'; /* remove the trailing comma */
    DOM_CAMERA_LOGI("camera region string '%s'\n", s);
    
    mParams.set(key, s);
    delete[] s;
  }
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
  char d[32];

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
  
  /* height and width */
  if (snprintf(d, sizeof(d), "%dx%d", aTakePicture->mWidth, aTakePicture->mHeight) > 0) {
    DOM_CAMERA_LOGI("setting picture size to %s\n", d);
    SetParameter(CameraParameters::KEY_PICTURE_SIZE, d);
  } else {
    DOM_CAMERA_LOGE("failed to set picture size\n");
    return NS_ERROR_INVALID_ARG;
  }

  /* picture format */
  if (mFileFormat) {
    nsMemory::Free(const_cast<char*>(mFileFormat));
  }
  mFileFormat = ToNewCString(aTakePicture->mFileFormat);
  if (mFileFormat) {
    DOM_CAMERA_LOGI("setting picture file format to %s\n", mFileFormat);
    SetParameter(CameraParameters::KEY_PICTURE_FORMAT, mFileFormat);
  } else {
    DOM_CAMERA_LOGE("failed to set picture format, out of memory\n");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  /* convert 'rotation' to a positive value from 0..270 degrees, in steps of 90 */
  PRUint32 r = static_cast<PRUint32>(aTakePicture->mRotation);
  r %= 360;
  r += 45;
  r /= 90;
  r *= 90;
  if (snprintf(d, sizeof(d), "%d", r) > 0) {
    DOM_CAMERA_LOGI("setting picture rotation to %d degrees (mapped from %d)\n", r, aTakePicture->mRotation);
    SetParameter(CameraParameters::KEY_ROTATION, d);
  } else {
    DOM_CAMERA_LOGE("failed to set picture rotation\n");
    return NS_ERROR_UNEXPECTED;
  }

  /* add any specified positional information -- don't care if these fail */
  if (aTakePicture->mLatitudeSet) {
    if (snprintf(d, sizeof(d), "%f", aTakePicture->mLatitude) > 0) {
      DOM_CAMERA_LOGI("setting picture latitude to %s\n", d);
      SetParameter(CameraParameters::KEY_GPS_LATITUDE, d);
    } else {
      DOM_CAMERA_LOGW("failed to set picture latitude\n");
    }
  }
  if (aTakePicture->mLongitudeSet) {
    if (snprintf(d, sizeof(d), "%f", aTakePicture->mLongitude) > 0) {
      DOM_CAMERA_LOGI("setting picture longitude to %s\n", d);
      SetParameter(CameraParameters::KEY_GPS_LONGITUDE, d);
    } else {
      DOM_CAMERA_LOGW("failed to set picture longitude\n");
    }
  }
  if (aTakePicture->mAltitudeSet) {
    if (snprintf(d, sizeof(d), "%f", aTakePicture->mAltitude) > 0) {
      DOM_CAMERA_LOGI("setting picture altitude to %s\n", d);
      SetParameter(CameraParameters::KEY_GPS_ALTITUDE, d);
    } else {
      DOM_CAMERA_LOGW("failed to set picture altitude\n");
    }
  }
  if (aTakePicture->mTimestampSet) {
    if (snprintf(d, sizeof(d), "%f", aTakePicture->mTimestamp) > 0) {
      DOM_CAMERA_LOGI("setting picture timestamp to %s\n", d);
      SetParameter(CameraParameters::KEY_GPS_TIMESTAMP, d);
    } else {
      DOM_CAMERA_LOGW("failed to set picture timestamp\n");
    }
  }

  if (GonkCameraHardware::doCameraHardwareTakePicture(mHwHandle) == OK) {
    return NS_OK;
  } else {
    return NS_ERROR_FAILURE;
  }
}

nsresult
nsCameraControl::DoPushParameters(PushParametersTask *aPushParameters)
{
  if (GonkCameraHardware::doCameraHardwarePushParameters(mHwHandle, mParams) == OK) {
    return NS_OK;
  } else {
    return NS_ERROR_FAILURE;
  }
}

nsresult
nsCameraControl::DoPullParameters(PullParametersTask *aPullParameters)
{
  GonkCameraHardware::doCameraHardwarePullParameters(mHwHandle, mParams);
  return NS_OK;
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

/*
  Gonk callback handlers.
*/
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
  gc->ReceiveFrame(aData, aLength);
}

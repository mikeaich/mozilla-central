/*
 * Copyright (C) 2012 Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include "libcameraservice/CameraHardwareInterface.h"
#include "camera/CameraParameters.h"
#include "nsCOMPtr.h"
#include "nsDOMClassInfo.h"
#include "nsMemory.h"
#include "jsapi.h"
#include "nsThread.h"
#include "nsPrintfCString.h"
#include "DOMCameraManager.h"
#include "GonkCameraHwMgr.h"
#include "CameraCapabilities.h"
#include "GonkCameraControl.h"
#include "GonkCameraPreview.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;

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

// Gonk-specific CameraControl implementation.

nsGonkCameraControl::nsGonkCameraControl(PRUint32 aCameraId, nsIThread *aCameraThread)
  : nsCameraControl(aCameraId, aCameraThread)
  , mHwHandle(0)
  , mExposureCompensationMin(0.0)
  , mExposureCompensationStep(0.0)
  , mDeferConfigUpdate(false)
{
  // Constructor runs on the camera thread--see DOMCameraManager.cpp::GetCameraImpl().
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
  mHwHandle = GonkCameraHardware::GetHandle(this, mCameraId);
  DOM_CAMERA_LOGI("%s:%d : this = %p, mHwHandle = %d\n", __func__, __LINE__, this, mHwHandle);

  // Initialize our camera configuration database.
  mRwLock = PR_NewRWLock(PR_RWLOCK_RANK_NONE, "GonkCameraControl.Parameters.Lock");
  PullParametersImpl(nsnull);

  // Grab any settings we'll need later.
  mExposureCompensationMin = mParams.getFloat(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION);
  mExposureCompensationStep = mParams.getFloat(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP);
  mMaxMeteringAreas = mParams.getInt(CameraParameters::KEY_MAX_NUM_METERING_AREAS);
  mMaxFocusAreas = mParams.getInt(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS);

  DOM_CAMERA_LOGI("minimum exposure compensation = %f\n", mExposureCompensationMin);
  DOM_CAMERA_LOGI("exposure compensation step = %f\n", mExposureCompensationStep);
  DOM_CAMERA_LOGI("maximum metering areas = %d\n", mMaxMeteringAreas);
  DOM_CAMERA_LOGI("maximum focus areas = %d\n", mMaxFocusAreas);
}

nsGonkCameraControl::~nsGonkCameraControl()
{
  DOM_CAMERA_LOGI("%s:%d : this = %p, mHwHandle = %d\n", __func__, __LINE__, this, mHwHandle);
  GonkCameraHardware::ReleaseHandle(mHwHandle);
  if (mRwLock) {
    PRRWLock *lock = mRwLock;
    mRwLock = nsnull;
    PR_DestroyRWLock(lock);
  }

  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
}

class RwAutoLockRead
{
public:
  RwAutoLockRead(PRRWLock* aRwLock)
    : mRwLock(aRwLock)
  {
    PR_RWLock_Rlock(mRwLock);
  }

  ~RwAutoLockRead()
  {
    PR_RWLock_Unlock(mRwLock);
  }

protected:
  PRRWLock *mRwLock;
};

class RwAutoLockWrite
{
public:
  RwAutoLockWrite(PRRWLock* aRwLock)
    : mRwLock(aRwLock)
  {
    PR_RWLock_Wlock(mRwLock);
  }

  ~RwAutoLockWrite()
  {
    PR_RWLock_Unlock(mRwLock);
  }

protected:
  PRRWLock *mRwLock;
};

const char*
nsGonkCameraControl::GetParameter(const char *aKey)
{
  RwAutoLockRead lock(mRwLock);
  return mParams.get(aKey);
}

const char*
nsGonkCameraControl::GetParameterConstChar(PRUint32 aKey)
{
  const char *key = getKeyText(aKey);
  if (!key) {
    return nsnull;
  }

  RwAutoLockRead lock(mRwLock);
  return mParams.get(key);
}

double
nsGonkCameraControl::GetParameterDouble(PRUint32 aKey)
{
  double val;
  int index = 0;
  double focusDistance[3];
  const char *s;

  const char *key = getKeyText(aKey);
  if (!key) {
    // return 1x when zooming is not supported
    return aKey == CAMERA_PARAM_ZOOM ? 1.0 : 0.0;
  }

  RwAutoLockRead lock(mRwLock);
  switch (aKey) {
    case CAMERA_PARAM_ZOOM:
      val = mParams.getInt(key);
      return val / 100;

    /**
     * The gonk camera parameters API only exposes one focus distance property
     * that contains "Near,Optimum,Far" distances, in metres, where 'Far' may
     * be 'Infinity'.
     */
    case CAMERA_PARAM_FOCUSDISTANCEFAR:
      ++index;
      // intentional fallthrough

    case CAMERA_PARAM_FOCUSDISTANCEOPTIMUM:
      ++index;
      // intentional fallthrough

    case CAMERA_PARAM_FOCUSDISTANCENEAR:
      s = mParams.get(key);
      if (sscanf(s, "%lf,%lf,%lf", &focusDistance[0], &focusDistance[1], &focusDistance[2]) == 3) {
        return focusDistance[index];
      }
      return 0.0;

    default:
      return mParams.getFloat(key);
  }
}

void
nsGonkCameraControl::GetParameter(PRUint32 aKey, CameraRegion **aRegions, PRUint32 *aLength)
{
  const char* key = getKeyText(aKey);
  if (!key) {
    *aRegions = nsnull;
    *aLength = 0;
    return;
  }

  RwAutoLockRead lock(mRwLock);

  const char* value = mParams.get(key);
  const char* p = value;
  PRUint32 count = 1;

  DOM_CAMERA_LOGI("key='%s' --> value='%s'\n", key, value);

  if (!value) {
    *aRegions = nsnull;
    *aLength = 0;
    return;
  }

  // count the number of regions in the string
  while ((p = strstr(p, "),("))) {
    ++count;
    p += 3;
  }

  CameraRegion *regions = new CameraRegion[count];
  CameraRegion *r;

  // parse all of the region sets
  PRUint32 i;
  for (i = 0, p = value; p && i < count; ++i, p = strchr(p + 1, '(')) {
    r = &regions[i];
    if (sscanf(p, "(%d,%d,%d,%d,%u)", &r->mTop, &r->mLeft, &r->mBottom, &r->mRight, &r->mWeight) != 5) {
      DOM_CAMERA_LOGE("%s:%d : region tuple has bad format: '%s'\n", __func__, __LINE__, p);
      goto GetParameter_error;
    }
  }

  *aRegions = regions;
  *aLength = count;
  return;

GetParameter_error:
  delete[] regions;
  *aRegions = nsnull;
  *aLength = 0;
}

void
nsGonkCameraControl::PushParameters()
{
  if (!mDeferConfigUpdate) {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    /**
     * If we're already on the camera thread, call PushParametersImpl()
     * directly, so that it executes synchronously.  Some callers
     * require this so that changes take effect immediately before
     * we can proceed.
     */
    if (NS_IsMainThread()) {
      nsCOMPtr<nsIRunnable> pushParametersTask = new PushParametersTask(this);
      mCameraThread->Dispatch(pushParametersTask, NS_DISPATCH_NORMAL);
    } else {
      PushParametersImpl(nsnull);
    }
  }
}

void
nsGonkCameraControl::SetParameter(const char *aKey, const char *aValue)
{
  {
    RwAutoLockWrite lock(mRwLock);
    mParams.set(aKey, aValue);
  }
  PushParameters();
}

void
nsGonkCameraControl::SetParameter(PRUint32 aKey, const char *aValue)
{
  const char *key = getKeyText(aKey);
  if (!key) {
    return;
  }

  {
    RwAutoLockWrite lock(mRwLock);
    mParams.set(key, aValue);
  }
  PushParameters();
}

void
nsGonkCameraControl::SetParameter(PRUint32 aKey, double aValue)
{
  PRUint32 index;

  const char *key = getKeyText(aKey);
  if (!key) {
    return;
  }

  {
    RwAutoLockWrite lock(mRwLock);
    if (aKey == CAMERA_PARAM_EXPOSURECOMPENSATION) {
      /**
       * Convert from real value to a Gonk index, round
       * to the nearest step; index is 1-based.
       */
      index = (aValue - mExposureCompensationMin + mExposureCompensationStep / 2) / mExposureCompensationStep + 1;
      DOM_CAMERA_LOGI("compensation = %f --> index = %d\n", aValue, index);
      mParams.set(key, index);
    } else {
      mParams.setFloat(key, aValue);
    }
  }
  PushParameters();
}

void
nsGonkCameraControl::SetParameter(PRUint32 aKey, CameraRegion *aRegions, PRUint32 aLength)
{
  const char *key = getKeyText(aKey);
  if (!key) {
    return;
  }

  if (!aLength) {
    // This tells the camera driver to revert to automatic regioning.
    mParams.set(key, "(0,0,0,0,0)");
    PushParameters();
    return;
  }

  nsCString s;

  for (PRUint32 i = 0; i < aLength; ++i) {
    CameraRegion *r = &aRegions[i];
    s.AppendPrintf("(%d,%d,%d,%d,%d),", r->mTop, r->mLeft, r->mBottom, r->mRight, r->mWeight);
  }

  // remove the trailing comma
  s.Trim(",", false, true, true);

  DOM_CAMERA_LOGI("camera region string '%s'\n", s.get());

  {
    RwAutoLockWrite lock(mRwLock);
    mParams.set(key, s.get());
  }
  PushParameters();
}

nsresult
nsGonkCameraControl::GetPreviewStreamImpl(GetPreviewStreamTask *aGetPreviewStream)
{
  nsCOMPtr<CameraPreview> preview = mPreview;
  nsresult rv;

  if (!preview) {
    preview = new GonkCameraPreview(mHwHandle, aGetPreviewStream->mWidth, aGetPreviewStream->mHeight);
    if (!preview) {
      rv = NS_DispatchToMainThread(new CameraErrorResult(aGetPreviewStream->mOnErrorCb, NS_LITERAL_STRING("OUT_OF_MEMORY")));
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  mPreview = preview;
  return NS_DispatchToMainThread(new GetPreviewStreamResult(preview.get(), aGetPreviewStream->mOnSuccessCb));
}

nsresult
nsGonkCameraControl::AutoFocusImpl(AutoFocusTask *aAutoFocus)
{
  nsCOMPtr<nsICameraAutoFocusCallback> cb = mAutoFocusOnSuccessCb;
  if (cb) {
    /**
     * We already have a callback, so someone has already
     * called autoFocus() -- cancel it.
     */
    mAutoFocusOnSuccessCb = nsnull;
    nsCOMPtr<nsICameraErrorCallback> ecb = mAutoFocusOnErrorCb;
    mAutoFocusOnErrorCb = nsnull;
    if (ecb) {
      nsresult rv = NS_DispatchToMainThread(new CameraErrorResult(ecb, NS_LITERAL_STRING("CANCELLED")));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    GonkCameraHardware::CancelAutoFocus(mHwHandle);
  }

  mAutoFocusOnSuccessCb = aAutoFocus->mOnSuccessCb;
  mAutoFocusOnErrorCb = aAutoFocus->mOnErrorCb;

  if (GonkCameraHardware::AutoFocus(mHwHandle) != OK) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
nsGonkCameraControl::TakePictureImpl(TakePictureTask *aTakePicture)
{
 nsCOMPtr<nsICameraTakePictureCallback> cb = mTakePictureOnSuccessCb;
  if (cb) {
    /**
     * We already have a callback, so someone has already
     * called TakePicture() -- cancel it.
     */
    mTakePictureOnSuccessCb = nsnull;
    nsCOMPtr<nsICameraErrorCallback> ecb = mTakePictureOnErrorCb;
    mTakePictureOnErrorCb = nsnull;
    if (ecb) {
      nsresult rv = NS_DispatchToMainThread(new CameraErrorResult(ecb, NS_LITERAL_STRING("CANCELLED")));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    GonkCameraHardware::CancelTakePicture(mHwHandle);
  }

  mTakePictureOnSuccessCb = aTakePicture->mOnSuccessCb;
  mTakePictureOnErrorCb = aTakePicture->mOnErrorCb;

  // batch-update camera configuration
  mDeferConfigUpdate = true;

  /**
   * height and width: some drivers are less friendly about getting one of
   * these set to zero, so if either is not specified, ignore both and go
   * with current or default settings.
   */
  if (aTakePicture->mWidth && aTakePicture->mHeight) {
    nsCString s;
    s.AppendPrintf("%dx%d", aTakePicture->mWidth, aTakePicture->mHeight);
    DOM_CAMERA_LOGI("setting picture size to '%s'\n", s.get());
    SetParameter(CameraParameters::KEY_PICTURE_SIZE, s.get());
  }

  // Picture format -- need to keep it for the callback.
  if (mFileFormat) {
    nsMemory::Free(const_cast<char*>(mFileFormat));
  }
  mFileFormat = ToNewCString(aTakePicture->mFileFormat);
  NS_ENSURE_TRUE(!!mFileFormat, NS_ERROR_OUT_OF_MEMORY);
  SetParameter(CameraParameters::KEY_PICTURE_FORMAT, mFileFormat);

  // Convert 'rotation' to a positive value from 0..270 degrees, in steps of 90.
  PRUint32 r = static_cast<PRUint32>(aTakePicture->mRotation);
  r %= 360;
  r += 45;
  r /= 90;
  r *= 90;
  DOM_CAMERA_LOGI("setting picture rotation to %d degrees (mapped from %d)\n", r, aTakePicture->mRotation);
  SetParameter(CameraParameters::KEY_ROTATION, nsPrintfCString("%u", r).get());

  // Add any specified positional information -- don't care if these fail.
  if (aTakePicture->mLatitudeSet) {
    DOM_CAMERA_LOGI("setting picture latitude to %lf\n", aTakePicture->mLatitude);
    SetParameter(CameraParameters::KEY_GPS_LATITUDE, nsPrintfCString("%lf", aTakePicture->mLatitude).get());
  }
  if (aTakePicture->mLongitudeSet) {
    DOM_CAMERA_LOGI("setting picture longitude to %lf\n", aTakePicture->mLongitude);
    SetParameter(CameraParameters::KEY_GPS_LONGITUDE, nsPrintfCString("%lf", aTakePicture->mLongitude).get());
  }
  if (aTakePicture->mAltitudeSet) {
    DOM_CAMERA_LOGI("setting picture altitude to %lf\n", aTakePicture->mAltitude);
    SetParameter(CameraParameters::KEY_GPS_ALTITUDE, nsPrintfCString("%lf", aTakePicture->mAltitude).get());
  }
  if (aTakePicture->mTimestampSet) {
    DOM_CAMERA_LOGI("setting picture timestamp to %lf\n", aTakePicture->mTimestamp);
    SetParameter(CameraParameters::KEY_GPS_TIMESTAMP, nsPrintfCString("%lf", aTakePicture->mTimestamp).get());
  }

  mDeferConfigUpdate = false;
  PushParameters();

  if (GonkCameraHardware::TakePicture(mHwHandle) != OK) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
nsGonkCameraControl::PushParametersImpl(PushParametersTask *aPushParameters)
{
  RwAutoLockRead lock(mRwLock);
  if (GonkCameraHardware::PushParameters(mHwHandle, mParams) != OK) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
nsGonkCameraControl::PullParametersImpl(PullParametersTask *aPullParameters)
{
  RwAutoLockWrite lock(mRwLock);
  GonkCameraHardware::PullParameters(mHwHandle, mParams);
  return NS_OK;
}

nsresult
nsGonkCameraControl::StartRecordingImpl(StartRecordingTask *aStartRecording)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsGonkCameraControl::StopRecordingImpl(StopRecordingTask *aStopRecording)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

void
nsGonkCameraControl::ReceiveFrame(PRUint8* aData, PRUint32 aLength)
{
  nsCOMPtr<CameraPreview> preview = mPreview;

  if (preview) {
    GonkCameraPreview *p = static_cast<GonkCameraPreview *>(preview.get());
    MOZ_ASSERT(p);
    p->ReceiveFrame(aData, aLength);
  }
}

// Gonk callback handlers.
namespace mozilla {

void
ReceiveImage(nsGonkCameraControl* gc, PRUint8* aData, PRUint32 aLength)
{
  gc->TakePictureComplete(aData, aLength);
}

void
AutoFocusComplete(nsGonkCameraControl* gc, bool success)
{
  gc->AutoFocusComplete(success);
}

void
ReceiveFrame(nsGonkCameraControl* gc, PRUint8* aData, PRUint32 aLength)
{
  gc->ReceiveFrame(aData, aLength);
}

} // namespace mozilla

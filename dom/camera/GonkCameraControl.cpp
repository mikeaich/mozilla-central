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
#include "DOMCameraCapabilities.h"
#include "GonkCameraControl.h"

#define DOM_CAMERA_DEBUG_REFS 1
#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;

static const char* getKeyText(PRUint32 aKey)
{
  switch (aKey) {
    case CAMERA_PARAM_EFFECT:
      return CameraParameters::KEY_EFFECT;
    case CAMERA_PARAM_WHITEBALANCE:
      return CameraParameters::KEY_WHITE_BALANCE;
    case CAMERA_PARAM_SCENEMODE:
      return CameraParameters::KEY_SCENE_MODE;
    case CAMERA_PARAM_FLASHMODE:
      return CameraParameters::KEY_FLASH_MODE;
    case CAMERA_PARAM_FOCUSMODE:
      return CameraParameters::KEY_FOCUS_MODE;
    case CAMERA_PARAM_ZOOM:
      return CameraParameters::KEY_ZOOM;
    case CAMERA_PARAM_METERINGAREAS:
      return CameraParameters::KEY_METERING_AREAS;
    case CAMERA_PARAM_FOCUSAREAS:
      return CameraParameters::KEY_FOCUS_AREAS;
    case CAMERA_PARAM_FOCALLENGTH:
      return CameraParameters::KEY_FOCAL_LENGTH;
    case CAMERA_PARAM_FOCUSDISTANCENEAR:
      return CameraParameters::KEY_FOCUS_DISTANCES;
    case CAMERA_PARAM_FOCUSDISTANCEOPTIMUM:
      return CameraParameters::KEY_FOCUS_DISTANCES;
    case CAMERA_PARAM_FOCUSDISTANCEFAR:
      return CameraParameters::KEY_FOCUS_DISTANCES;
    case CAMERA_PARAM_EXPOSURECOMPENSATION:
      return CameraParameters::KEY_EXPOSURE_COMPENSATION;
    case CAMERA_PARAM_SUPPORTED_PREVIEWSIZES:
      return CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES;
    case CAMERA_PARAM_SUPPORTED_VIDEOSIZES:
      return CameraParameters::KEY_SUPPORTED_VIDEO_SIZES;
    case CAMERA_PARAM_SUPPORTED_PICTURESIZES:
      return CameraParameters::KEY_SUPPORTED_PICTURE_SIZES;
    case CAMERA_PARAM_SUPPORTED_PICTUREFORMATS:
      return CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS;
    case CAMERA_PARAM_SUPPORTED_WHITEBALANCES:
      return CameraParameters::KEY_SUPPORTED_WHITE_BALANCE;
    case CAMERA_PARAM_SUPPORTED_SCENEMODES:
      return CameraParameters::KEY_SUPPORTED_SCENE_MODES;
    case CAMERA_PARAM_SUPPORTED_EFFECTS:
      return CameraParameters::KEY_SUPPORTED_EFFECTS;
    case CAMERA_PARAM_SUPPORTED_FLASHMODES:
      return CameraParameters::KEY_SUPPORTED_FLASH_MODES;
    case CAMERA_PARAM_SUPPORTED_FOCUSMODES:
      return CameraParameters::KEY_SUPPORTED_FOCUS_MODES;
    case CAMERA_PARAM_SUPPORTED_MAXFOCUSAREAS:
      return CameraParameters::KEY_MAX_NUM_FOCUS_AREAS;
    case CAMERA_PARAM_SUPPORTED_MAXMETERINGAREAS:
      return CameraParameters::KEY_MAX_NUM_METERING_AREAS;
    case CAMERA_PARAM_SUPPORTED_MINEXPOSURECOMPENSATION:
      return CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION;
    case CAMERA_PARAM_SUPPORTED_MAXEXPOSURECOMPENSATION:
      return CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION;
    case CAMERA_PARAM_SUPPORTED_EXPOSURECOMPENSATIONSTEP:
      return CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP;
    case CAMERA_PARAM_SUPPORTED_ZOOM:
      return CameraParameters::KEY_ZOOM_SUPPORTED;
    case CAMERA_PARAM_SUPPORTED_ZOOMRATIOS:
      return CameraParameters::KEY_ZOOM_RATIOS;
    default:
      return nullptr;
  }
}

// nsDOMCameraControl implementation-specific constructor
nsDOMCameraControl::nsDOMCameraControl(PRUint32 aCameraId, nsIThread* aCameraThread, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError)
  : mCameraId(aCameraId)
  , mCameraThread(aCameraThread)
  , mDOMCapabilities(nullptr)
  , mDOMPreview(nullptr)
{
  DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  mCameraControl = new nsGonkCameraControl(mCameraId, mCameraThread, this, onSuccess, onError);

  /**
   * nsDOMCameraControl is a cycle-collection participant, which means it is
   * not threadsafe--so we need to bump up its reference count here to make
   * sure that it exists long enough to be initialized.
   *
   * Once it is initialized, the main thread result runnable will decrement
   * it again to make sure it can be cleaned up.
   *
   * nsGonkCameraControl MUST NOT hold a strong reference to this
   * nsDOMCameraControl or memory will leak!
   */
  NS_ADDREF_THIS();
}

// Gonk-specific CameraControl implementation.

// Initialize nsGonkCameraControl instance--runs on camera thread.
class InitGonkCameraControl : public nsRunnable
{
public:
  InitGonkCameraControl(nsGonkCameraControl* aCameraControl, nsICameraControl* aDOMCameraControl, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError)
    : mCameraControl(aCameraControl)
    , mDOMCameraControl(aDOMCameraControl)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  { }

  NS_IMETHOD Run()
  {
    nsresult rv = mCameraControl->Init();
    rv = NS_DispatchToMainThread(new GetCameraResult(mDOMCameraControl, mCameraControl, rv, mOnSuccessCb, mOnErrorCb));
    if (NS_FAILED(rv)) {
      DOM_CAMERA_LOGE("%s: failed to dispatch camera result to main thread (%d), POSSIBLE MEMORY LEAK!\n", rv);
    }
    return rv;
  }

  nsRefPtr<nsGonkCameraControl> mCameraControl;
  // Raw pointer to DOM-facing camera control--it must NS_ADDREF itself for us
  nsICameraControl* mDOMCameraControl;
  nsCOMPtr<nsICameraGetCameraCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};

// Construct nsGonkCameraControl on the main thread.
nsGonkCameraControl::nsGonkCameraControl(PRUint32 aCameraId, nsIThread* aCameraThread, nsICameraControl* aDOMCameraControl, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError)
  : CameraControl(aCameraId, aCameraThread)
  , mHwHandle(0)
  , mExposureCompensationMin(0.0)
  , mExposureCompensationStep(0.0)
  , mDeferConfigUpdate(false)
  , mWidth(0)
  , mHeight(0)
  , mFormat(GonkCameraHardware::PREVIEW_FORMAT_UNKNOWN)
  , mDiscardedFrameCount(0)
{
  // Constructor runs on the main thread...
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
  mRwLock = PR_NewRWLock(PR_RWLOCK_RANK_NONE, "GonkCameraControl.Parameters.Lock");

  // ...but initialization is carried out on the camera thread.
  nsCOMPtr<nsIRunnable> init = new InitGonkCameraControl(this, aDOMCameraControl, onSuccess, onError);
  mCameraThread->Dispatch(init, NS_DISPATCH_NORMAL);
}

nsresult
nsGonkCameraControl::Init()
{
  mHwHandle = GonkCameraHardware::GetHandle(this, mCameraId);
  DOM_CAMERA_LOGI("Initializing camera %d (this = %p, mHwHandle = %d)\n", mCameraId, this, mHwHandle);

  // Initialize our camera configuration database.
  PullParametersImpl(nullptr);

  // Grab any settings we'll need later.
  mExposureCompensationMin = mParams.getFloat(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION);
  mExposureCompensationStep = mParams.getFloat(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP);
  mMaxMeteringAreas = mParams.getInt(CameraParameters::KEY_MAX_NUM_METERING_AREAS);
  mMaxFocusAreas = mParams.getInt(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS);

  DOM_CAMERA_LOGI(" - minimum exposure compensation: %f\n", mExposureCompensationMin);
  DOM_CAMERA_LOGI(" - exposure compensation step:    %f\n", mExposureCompensationStep);
  DOM_CAMERA_LOGI(" - maximum metering areas:        %d\n", mMaxMeteringAreas);
  DOM_CAMERA_LOGI(" - maximum focus areas:           %d\n", mMaxFocusAreas);

  return mHwHandle != 0 ? NS_OK : NS_ERROR_FAILURE;
}

nsGonkCameraControl::~nsGonkCameraControl()
{
  DOM_CAMERA_LOGI("%s:%d : this = %p, mHwHandle = %d\n", __func__, __LINE__, this, mHwHandle);
  GonkCameraHardware::ReleaseHandle(mHwHandle);
  if (mRwLock) {
    PRRWLock* lock = mRwLock;
    mRwLock = nullptr;
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
  PRRWLock* mRwLock;
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
  PRRWLock* mRwLock;
};

const char*
nsGonkCameraControl::GetParameter(const char* aKey)
{
  RwAutoLockRead lock(mRwLock);
  return mParams.get(aKey);
}

const char*
nsGonkCameraControl::GetParameterConstChar(PRUint32 aKey)
{
  const char* key = getKeyText(aKey);
  if (!key) {
    return nullptr;
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
  const char* s;

  const char* key = getKeyText(aKey);
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

    case CAMERA_PARAM_EXPOSURECOMPENSATION:
      index = mParams.getInt(key);
      if (!index) {
        // NaN indicates automatic exposure compensation
        return NAN;
      }
      val = (index - 1) * mExposureCompensationStep + mExposureCompensationMin;
      DOM_CAMERA_LOGI("index = %d --> compensation = %f\n", index, val);
      return val;

    default:
      return mParams.getFloat(key);
  }
}

void
nsGonkCameraControl::GetParameter(PRUint32 aKey, nsTArray<CameraRegion>& aRegions)
{
  aRegions.Clear();

  const char* key = getKeyText(aKey);
  if (!key) {
    return;
  }

  RwAutoLockRead lock(mRwLock);

  const char* value = mParams.get(key);
  DOM_CAMERA_LOGI("key='%s' --> value='%s'\n", key, value);
  if (!value) {
    return;
  }

  const char* p = value;
  PRUint32 count = 1;

  // count the number of regions in the string
  while ((p = strstr(p, "),("))) {
    ++count;
    p += 3;
  }

  aRegions.SetCapacity(count);
  CameraRegion* r;

  // parse all of the region sets
  PRUint32 i;
  for (i = 0, p = value; p && i < count; ++i, p = strchr(p + 1, '(')) {
    r = aRegions.AppendElement();
    if (sscanf(p, "(%d,%d,%d,%d,%u)", &r->top, &r->left, &r->bottom, &r->right, &r->weight) != 5) {
      DOM_CAMERA_LOGE("%s:%d : region tuple has bad format: '%s'\n", __func__, __LINE__, p);
      goto GetParameter_error;
    }
  }

  return;

GetParameter_error:
  aRegions.Clear();
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
      PushParametersImpl(nullptr);
    }
  }
}

void
nsGonkCameraControl::SetParameter(const char* aKey, const char* aValue)
{
  {
    RwAutoLockWrite lock(mRwLock);
    mParams.set(aKey, aValue);
  }
  PushParameters();
}

void
nsGonkCameraControl::SetParameter(PRUint32 aKey, const char* aValue)
{
  const char* key = getKeyText(aKey);
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

  const char* key = getKeyText(aKey);
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
nsGonkCameraControl::SetParameter(PRUint32 aKey, const nsTArray<CameraRegion>& aRegions)
{
  const char* key = getKeyText(aKey);
  if (!key) {
    return;
  }

  PRUint32 length = aRegions.Length();

  if (!length) {
    // This tells the camera driver to revert to automatic regioning.
    mParams.set(key, "(0,0,0,0,0)");
    PushParameters();
    return;
  }

  nsCString s;

  for (PRUint32 i = 0; i < length; ++i) {
    const CameraRegion* r = &aRegions[i];
    s.AppendPrintf("(%d,%d,%d,%d,%d),", r->top, r->left, r->bottom, r->right, r->weight);
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
nsGonkCameraControl::AutoFocusImpl(AutoFocusTask* aAutoFocus)
{
  nsCOMPtr<nsICameraAutoFocusCallback> cb = mAutoFocusOnSuccessCb;
  if (cb) {
    /**
     * We already have a callback, so someone has already
     * called autoFocus() -- cancel it.
     */
    mAutoFocusOnSuccessCb = nullptr;
    nsCOMPtr<nsICameraErrorCallback> ecb = mAutoFocusOnErrorCb;
    mAutoFocusOnErrorCb = nullptr;
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
nsGonkCameraControl::TakePictureImpl(TakePictureTask* aTakePicture)
{
 nsCOMPtr<nsICameraTakePictureCallback> cb = mTakePictureOnSuccessCb;
  if (cb) {
    /**
     * We already have a callback, so someone has already
     * called TakePicture() -- cancel it.
     */
    mTakePictureOnSuccessCb = nullptr;
    nsCOMPtr<nsICameraErrorCallback> ecb = mTakePictureOnErrorCb;
    mTakePictureOnErrorCb = nullptr;
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
  if (aTakePicture->mSize.width && aTakePicture->mSize.height) {
    nsCString s;
    s.AppendPrintf("%dx%d", aTakePicture->mSize.width, aTakePicture->mSize.height);
    DOM_CAMERA_LOGI("setting picture size to '%s'\n", s.get());
    SetParameter(CameraParameters::KEY_PICTURE_SIZE, s.get());
  }

  // Picture format -- need to keep it for the callback.
  mFileFormat = aTakePicture->mFileFormat;
  SetParameter(CameraParameters::KEY_PICTURE_FORMAT, NS_ConvertUTF16toUTF8(mFileFormat).get());

  // Convert 'rotation' to a positive value from 0..270 degrees, in steps of 90.
  PRUint32 r = static_cast<PRUint32>(aTakePicture->mRotation);
  r %= 360;
  r += 45;
  r /= 90;
  r *= 90;
  DOM_CAMERA_LOGI("setting picture rotation to %d degrees (mapped from %d)\n", r, aTakePicture->mRotation);
  SetParameter(CameraParameters::KEY_ROTATION, nsPrintfCString("%u", r).get());

  // Add any specified positional information -- don't care if these fail.
  if (!isnan(aTakePicture->mPosition.latitude)) {
    DOM_CAMERA_LOGI("setting picture latitude to %lf\n", aTakePicture->mPosition.latitude);
    SetParameter(CameraParameters::KEY_GPS_LATITUDE, nsPrintfCString("%lf", aTakePicture->mPosition.latitude).get());
  }
  if (!isnan(aTakePicture->mPosition.longitude)) {
    DOM_CAMERA_LOGI("setting picture longitude to %lf\n", aTakePicture->mPosition.longitude);
    SetParameter(CameraParameters::KEY_GPS_LONGITUDE, nsPrintfCString("%lf", aTakePicture->mPosition.longitude).get());
  }
  if (!isnan(aTakePicture->mPosition.altitude)) {
    DOM_CAMERA_LOGI("setting picture altitude to %lf\n", aTakePicture->mPosition.altitude);
    SetParameter(CameraParameters::KEY_GPS_ALTITUDE, nsPrintfCString("%lf", aTakePicture->mPosition.altitude).get());
  }
  if (!isnan(aTakePicture->mPosition.timestamp)) {
    DOM_CAMERA_LOGI("setting picture timestamp to %lf\n", aTakePicture->mPosition.timestamp);
    SetParameter(CameraParameters::KEY_GPS_TIMESTAMP, nsPrintfCString("%lf", aTakePicture->mPosition.timestamp).get());
  }

  mDeferConfigUpdate = false;
  PushParameters();

  if (GonkCameraHardware::TakePicture(mHwHandle) != OK) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
nsGonkCameraControl::PushParametersImpl(PushParametersTask* aPushParameters)
{
  RwAutoLockRead lock(mRwLock);
  if (GonkCameraHardware::PushParameters(mHwHandle, mParams) != OK) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
nsGonkCameraControl::PullParametersImpl(PullParametersTask* aPullParameters)
{
  RwAutoLockWrite lock(mRwLock);
  GonkCameraHardware::PullParameters(mHwHandle, mParams);
  return NS_OK;
}

nsresult
nsGonkCameraControl::StartRecordingImpl(StartRecordingTask* aStartRecording)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsGonkCameraControl::StopRecordingImpl(StopRecordingTask* aStopRecording)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsGonkCameraControl::GetPreviewStreamImpl(GetPreviewStreamTask* aGetPreviewStream)
{
  DOM_CAMERA_LOGI("%s: configuring preview\n", __func__);

  /**
   * We set and then immediately get the preview size in case the camera
   * driver has decided to ignore our desired dimensions and substitute its
   * own.  We need to know the dimensions the driver is using so that, if
   * needed, we can properly de-interlace the yuv420sp format in
   * ReceiveFrame().
   */
  GonkCameraHardware::SetPreviewSize(mHwHandle, aGetPreviewStream->mSize.width, aGetPreviewStream->mSize.height);
  GonkCameraHardware::GetPreviewSize(mHwHandle, &mWidth, &mHeight);

  PRUint32 fps = GonkCameraHardware::GetFps(mHwHandle);
  mFormat = GonkCameraHardware::GetPreviewFormat(mHwHandle);

  DOM_CAMERA_LOGI("wanted preview %d x %d, got %d x %d (%d fps, format %d)\n", aGetPreviewStream->mSize.width, aGetPreviewStream->mSize.height, mWidth, mHeight, fps, mFormat);

  nsCOMPtr<GetPreviewStreamResult> getPreviewStreamResult = new GetPreviewStreamResult(this, mWidth, mHeight, fps, aGetPreviewStream->mOnSuccessCb);
  return NS_DispatchToMainThread(getPreviewStreamResult);
}

nsresult
nsGonkCameraControl::StartPreviewImpl(StartPreviewTask* aStartPreview)
{
  if (mDOMPreview) {
    mDOMPreview->Stopped(true);
  }
  mDOMPreview = aStartPreview->mDOMPreview;

  DOM_CAMERA_LOGI("%s: starting preview (mDOMPreview=%p)\n", __func__, mDOMPreview);
  if (GonkCameraHardware::StartPreview(mHwHandle) != OK) {
    DOM_CAMERA_LOGE("%s: failed to start preview\n", __func__);
    return NS_ERROR_FAILURE;
  }

  mDOMPreview->Started();
  return NS_OK;
}

nsresult
nsGonkCameraControl::StopPreviewImpl(StopPreviewTask* aStopPreview)
{
  DOM_CAMERA_LOGI("%s: stopping preview\n", __func__);

  // StopPreview() is a synchronous call--it doesn't return
  // until the camera preview thread exits.
  GonkCameraHardware::StopPreview(mHwHandle);
  mDOMPreview->Stopped();
  mDOMPreview = nullptr;

  return NS_OK;
}

/**
 * This big macro takes two 32-bit input blocks of interlaced u and
 * v data (from a yuv420sp frame) in 's0' and 's1', and deinterlaces
 * them into pairs of contiguous 32-bit blocks, the u plane data in
 * 'u', and the v plane data in 'v' (i.e. for a yuv420p frame).
 *
 * yuv420sp:
 *  [ y-data ][ uv-data ]
 *    [ uv-data ]: [u0][v0][u1][v1][u2][v2]...
 *
 * yuv420p:
 *  [ y-data ][ u-data ][ v-data ]
 *    [ u-data ]: [u0][u1][u2]...
 *    [ v-data ]: [v0][v1][v2]...
 *
 * Doing this in 32-bit blocks is significantly faster than using
 * byte-wise operations on ARM.  (In some cases, the byte-wise
 * de-interlacing can be too slow to keep up with the preview frames
 * coming from the driver.
 */
#define DEINTERLACE( u, v, s0, s1 )                             \
  u = ( (s0) & 0xFF00UL ) >> 8 | ( (s0) & 0xFF000000UL ) >> 16; \
  u |= ( (s1) & 0xFF00UL ) << 8 | ( (s1) & 0xFF000000UL );      \
  v = ( (s0) & 0xFFUL ) | ( (s0) & 0xFF0000UL ) >> 8;           \
  v |= ( (s1) & 0xFFUL ) << 16 | ( (s1) & 0xFF0000UL ) << 8;

void
nsGonkCameraControl::ReceiveFrame(PRUint8* aData, PRUint32 aLength)
{
  DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);

  if (mDOMPreview->HaveEnoughBuffered()) {
    if (mDiscardedFrameCount == 0) {
      DOM_CAMERA_LOGI("mInput has enough data buffered, starting to discard\n");
    }
    ++mDiscardedFrameCount;
    return;
  } else if (mDiscardedFrameCount) {
    DOM_CAMERA_LOGI("mInput needs more data again; discarded %d frames in a row\n", mDiscardedFrameCount);
    mDiscardedFrameCount = 0;
  }

  switch (mFormat) {
    case GonkCameraHardware::PREVIEW_FORMAT_YUV420SP:
      {
        // de-interlace the u and v planes
        uint8_t* y = aData;
        uint32_t yN = mWidth * mHeight;

        NS_ASSERTION((yN & 0x3) == 0, "Invalid image dimensions!");

        uint32_t uvN = yN / 4;
        uint32_t* src = (uint32_t*)( y + yN );
        uint32_t* d = new uint32_t[ uvN / 2 ];
        uint32_t* u = d;
        uint32_t* v = u + uvN / 4;

        // we're handling pairs of 32-bit words, so divide by 8
        NS_ASSERTION((uvN & 0x7) == 0, "Invalid image dimensions!");
        uvN /= 8;

        while (uvN--) {
          uint32_t src0 = *src++;
          uint32_t src1 = *src++;

          uint32_t u0;
          uint32_t v0;
          uint32_t u1;
          uint32_t v1;

          DEINTERLACE( u0, v0, src0, src1 );

          src0 = *src++;
          src1 = *src++;

          DEINTERLACE( u1, v1, src0, src1 );

          *u++ = u0;
          *u++ = u1;
          *v++ = v0;
          *v++ = v1;
        }

        memcpy(y + yN, d, yN / 2);
        delete[] d;
      }
      break;

    case GonkCameraHardware::PREVIEW_FORMAT_YUV420P:
      // no transformation required
      break;

    default:
      // in a format we don't handle, get out of here
      return;
  }

  CameraControl::ReceiveFrame(aData);
}

void
nsGonkCameraControl::AutoFocusComplete(bool aSuccess)
{
  /**
   * Auto focusing can change some of the camera's parameters, so
   * we need to pull a new set before sending the result to the
   * main thread.
   */
  PullParametersImpl(nullptr);

  nsCOMPtr<nsIRunnable> autoFocusResult = new AutoFocusResult(aSuccess, mAutoFocusOnSuccessCb);

  nsresult rv = NS_DispatchToMainThread(autoFocusResult);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch autoFocus() onSuccess callback to main thread!");
  }
}

void
nsGonkCameraControl::TakePictureComplete(PRUint8* aData, PRUint32 aLength)
{
  PRUint8* data = new PRUint8[aLength];

  memcpy(data, aData, aLength);

  /**
   * TODO: pick up the actual specified picture format for the MIME type;
   * for now, assume we'll be using JPEGs.
   */
  nsIDOMBlob* blob = new nsDOMMemoryFile(static_cast<void*>(data), static_cast<PRUint64>(aLength), NS_LITERAL_STRING("image/jpeg"));
  nsCOMPtr<nsIRunnable> takePictureResult = new TakePictureResult(blob, mTakePictureOnSuccessCb);

  nsresult rv = NS_DispatchToMainThread(takePictureResult);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch takePicture() onSuccess callback to main thread!");
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
AutoFocusComplete(nsGonkCameraControl* gc, bool aSuccess)
{
  gc->AutoFocusComplete(aSuccess);
}

void
ReceiveFrame(nsGonkCameraControl* gc, PRUint8* aData, PRUint32 aLength)
{
  gc->ReceiveFrame(aData, aLength);
}

} // namespace mozilla

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
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "libcameraservice/CameraHardwareInterface.h"
#include "camera/CameraParameters.h"
#include "nsCOMPtr.h"
#include "nsDOMClassInfo.h"
#include "nsMemory.h"
#include "jsapi.h"
#include "nsThread.h"
#include <media/MediaProfiles.h>
#include "nsDirectoryServiceDefs.h" // for NS_GetSpecialDirectory
#include "DOMCameraManager.h"
#include "GonkCameraHwMgr.h"
#include "CameraCapabilities.h"
#include "GonkCameraControl.h"
#include "GonkCameraPreview.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"


USING_CAMERA_NAMESPACE

#define DEFAULT_VIDEO_STORAGE_TO_TEMP_DIR   1
#define MAX_VIDEO_FILE_NAME_LEN             256
#define VIDEO_STORAGE_DIR                   "/sdcard/Movies"
#define DEFAULT_VIDEO_FILE_NAME             "video"
#define DEFAULT_VIDEO_QUALITY               3 // cif:352x288

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

nsGonkCameraControl::nsGonkCameraControl(PRUint32 aCameraId, nsIThread *aCameraThread)
  : nsCameraControl(aCameraId, aCameraThread)
  , mHwHandle(0)
  , mExpsoureCompensationMin(0.0)
  , mExpsoureCompensationStep(0.0)
  , mDeferConfigUpdate(false)
{
  /* Constructor runs on the camera thread--see DOMCameraManager.cpp::DoGetCamera(). */
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
  mHwHandle = GonkCameraHardware::getCameraHardwareHandle(this, mCameraId);
  DOM_CAMERA_LOGI("%s:%d : this = %p, mHwHandle = %d\n", __func__, __LINE__, this, mHwHandle);

  /* Initialize our camera configuration database. */
  mRwLock = PR_NewRWLock(PR_RWLOCK_RANK_NONE, "GonkCameraControl.Parameters.Lock");
  DoPullParameters(nsnull);

  /* Grab any settings we'll need later */
  mExpsoureCompensationMin = mParams.getFloat(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION);
  mExpsoureCompensationStep = mParams.getFloat(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP);

  DOM_CAMERA_LOGI("minimum exposure compensation = %f\n", mExpsoureCompensationMin);
  DOM_CAMERA_LOGI("exposure compensation step = %f\n", mExpsoureCompensationStep);
}

nsGonkCameraControl::~nsGonkCameraControl()
{
  DOM_CAMERA_LOGI("%s:%d : this = %p, mHwHandle = %d\n", __func__, __LINE__, this, mHwHandle);
  GonkCameraHardware::releaseCameraHardwareHandle(mHwHandle);
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
  if (key) {
    RwAutoLockRead lock(mRwLock);
    return mParams.get(key);
  } else {
    return nsnull;
  }
}

double
nsGonkCameraControl::GetParameterDouble(PRUint32 aKey)
{
  double val;
  int index = 0;
  const char *s;
  RwAutoLockRead lock(mRwLock);

  const char *key = getKeyText(aKey);
  if (key) {
    switch (aKey) {
      case CAMERA_PARAM_ZOOM:
        val = mParams.getInt(key);
        return val / 100;

      case CAMERA_PARAM_FOCUSDISTANCEFAR:
        ++index;
        /* intentional fallthrough */

      case CAMERA_PARAM_FOCUSDISTANCEOPTIMUM:
        ++index;
        /* intentional fallthrough */

      case CAMERA_PARAM_FOCUSDISTANCENEAR:
        s = mParams.get(key);
        // DOM_CAMERA_LOGI("key='%s' --> value='%s'\n", key, s);
        if (!s) {
          return 0.0;
        }
        while(index) {
          s = strchr(s, ',');
          if (!s) {
            return 0.0;
          }
          ++s;
          --index;
        }
        val = strtod(s, nsnull);
        return val;

      default:
        return mParams.getFloat(key);
    }
  } else {
    if (aKey == CAMERA_PARAM_ZOOM) {
      /* return 1x when zooming is not supported */
      return 1.0;
    } else {
      return 0.0;
    }
  }
}

void
nsGonkCameraControl::GetParameter(PRUint32 aKey, CameraRegion **aRegions, PRUint32 *aLength)
{
  const char* key = getKeyText(aKey);
  RwAutoLockRead lock(mRwLock);

  if (!key) {
    *aRegions = nsnull;
    *aLength = 0;
    return;
  }

  const char* value = mParams.get(key);
  const char* p = value;
  PRUint32 count = 1;

  DOM_CAMERA_LOGI("key='%s' --> value='%s'\n", key, value);

  if (!value) {
    *aRegions = nsnull;
    *aLength = 0;
    return;
  }

  /* count the number of regions in the string */
  while ((p = strstr(p, "),("))) {
    ++count;
    p += 3;
  }

  CameraRegion *regions = new CameraRegion[count];
  CameraRegion *r;

  /* parse all of the region sets */
  char *end;
  p = value + 1;
  for (PRUint32 i = 0; i < count; ++i) {
    r = &regions[i];

    for (PRUint32 field = 0; field < 5; ++field) {
      PRInt32 v;
      if (field != 4) {
        /* dimension fields are signed */
        v = strtol(p, &end, 10);
      }
      switch (field) {
        case 0:
          r->mTop = v;
          break;

        case 1:
          r->mLeft = v;
          break;

        case 2:
          r->mBottom = v;
          break;

        case 3:
          r->mRight = v;
          break;

        case 4:
          /* weight value is unsigned */
          r->mWeight = strtoul(p, &end, 10);
          break;

        default:
          DOM_CAMERA_LOGE("%s:%d : should never reach here\n", __func__, __LINE__);
          goto GetParameter_error;
      }
      p = end;
      switch (*p) {
        case ')':
          if (field == 4) {
            /* end of this region */
            switch (*++p) {
              case ',':
                /* there are more regions */
                if (*(p + 1) == '(') {
                  p += 2;
                  continue;
                }
                break;

              case '\0':
                /* end of string, we're done */
                if (i + 1 != count) {
                  DOM_CAMERA_LOGE("%s:%d : region list parsed short\n", __func__, __LINE__);
                  count = i;
                }
                goto GetParameter_done;
            }
          }
          /* intentional fallthrough */

        default:
          DOM_CAMERA_LOGE("%s:%d : malformed region '%s'\n", __func__, __LINE__, p);
          goto GetParameter_error;

        case '\0':
          DOM_CAMERA_LOGE("%s:%d : abnormally short region group\n", __func__, __LINE__);
          goto GetParameter_error;

        case ',':
          if (field != 4) {
            ++p;
            break;
          }
          DOM_CAMERA_LOGE("%s:%d : abnormally long region group\n", __func__, __LINE__);
          goto GetParameter_error;
      }
    }
  }

GetParameter_done:
  *aRegions = regions;
  *aLength = count;
  return;

GetParameter_error:
  delete[] *aRegions;
  *aRegions = nsnull;
  *aLength = 0;
}

void
nsGonkCameraControl::PushParameters()
{
  if (!mDeferConfigUpdate) {
    DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
    if (NS_IsMainThread()) {
      nsCOMPtr<nsIRunnable> pushParametersTask = new PushParametersTask(this);
      mCameraThread->Dispatch(pushParametersTask, NS_DISPATCH_NORMAL);
    } else {
      DoPushParameters(nsnull);
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
  if (key) {
    {
      RwAutoLockWrite lock(mRwLock);
      mParams.set(key, aValue);
    }
    PushParameters();
  }
}

void
nsGonkCameraControl::SetParameter(PRUint32 aKey, double aValue)
{
  PRUint32 index;

  const char *key = getKeyText(aKey);
  if (key) {
    {
      RwAutoLockWrite lock(mRwLock);
      switch (aKey) {
        case CAMERA_PARAM_EXPOSURECOMPENSATION:
          /* convert from real value to a Gonk index, round to the nearest step; index is 1-based */
          index = (aValue - mExpsoureCompensationMin + mExpsoureCompensationStep / 2) / mExpsoureCompensationStep + 1;
          DOM_CAMERA_LOGI("compensation = %f --> index = %d\n", aValue, index);
          mParams.set(key, index);
          break;

        default:
          mParams.setFloat(key, aValue);
          break;
      }
    }
    PushParameters();
  }
}

void
nsGonkCameraControl::SetParameter(PRUint32 aKey, CameraRegion *aRegions, PRUint32 aLength)
{
  const char *key = getKeyText(aKey);
  if (key) {
    if (!aLength) {
      /* This tells the camera driver to revert to automatic regioning. */
      mParams.set(key, "(0,0,0,0,0)");
      PushParameters();
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

    *(p - 1) = '\0'; /* remove the trailing comma */
    DOM_CAMERA_LOGI("camera region string '%s'\n", s);

    {
      RwAutoLockWrite lock(mRwLock);
      mParams.set(key, s);
    }
    delete[] s;
    PushParameters();
  }
}

nsresult
nsGonkCameraControl::DoGetPreviewStream(GetPreviewStreamTask *aGetPreviewStream)
{
  nsCOMPtr<CameraPreview> preview = mPreview;

  if (!preview) {
    preview = new GonkCameraPreview(mHwHandle, aGetPreviewStream->mWidth, aGetPreviewStream->mHeight);
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
nsGonkCameraControl::DoAutoFocus(AutoFocusTask *aAutoFocus)
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
nsGonkCameraControl::DoTakePicture(TakePictureTask *aTakePicture)
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

  /* batch-update camera configuration */
  mDeferConfigUpdate = true;

  /* height and width: some drivers are less friendly about getting one of
     these set to zero, so if either is not specified, ignore both and go
     with current or default settings. */
  if (aTakePicture->mWidth && aTakePicture->mHeight) {
    if (snprintf(d, sizeof(d), "%dx%d", aTakePicture->mWidth, aTakePicture->mHeight) > 0) {
      DOM_CAMERA_LOGI("setting picture size to %s\n", d);
      SetParameter(CameraParameters::KEY_PICTURE_SIZE, d);
    } else {
      DOM_CAMERA_LOGE("failed to set picture size\n");
      return NS_ERROR_INVALID_ARG;
    }
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

  mDeferConfigUpdate = false;
  PushParameters();

  if (GonkCameraHardware::doCameraHardwareTakePicture(mHwHandle) == OK) {
    // mPreview->Stop();
    return NS_OK;
  } else {
    return NS_ERROR_FAILURE;
  }
}

nsresult
nsGonkCameraControl::DoPushParameters(PushParametersTask *aPushParameters)
{
  RwAutoLockRead lock(mRwLock);
  if (GonkCameraHardware::doCameraHardwarePushParameters(mHwHandle, mParams) == OK) {
    return NS_OK;
  } else {
    return NS_ERROR_FAILURE;
  }
}

nsresult
nsGonkCameraControl::DoPullParameters(PullParametersTask *aPullParameters)
{
  RwAutoLockWrite lock(mRwLock);
  GonkCameraHardware::doCameraHardwarePullParameters(mHwHandle, mParams);
  return NS_OK;
}

nsresult
nsGonkCameraControl::SetupVideoMode()
{
  // read perferences for video mode
  mMediaProfiles = MediaProfiles::getInstance();
  // we should probably get this from somewhere, right now just #define
  int quality = DEFAULT_VIDEO_QUALITY;
  camcorder_quality q = static_cast<camcorder_quality>(quality);
  mDuration         = mMediaProfiles->getCamcorderProfileParamByName("duration",    (int)mCameraId, q);
  mVideoFileFormat  = mMediaProfiles->getCamcorderProfileParamByName("file.format", (int)mCameraId, q);
  mVideoCodec       = mMediaProfiles->getCamcorderProfileParamByName("vid.codec",   (int)mCameraId, q);
  mVideoBitRate     = mMediaProfiles->getCamcorderProfileParamByName("vid.bps",     (int)mCameraId, q);
  mVideoFrameRate   = mMediaProfiles->getCamcorderProfileParamByName("vid.fps",     (int)mCameraId, q);
  mVideoFrameWidth  = mMediaProfiles->getCamcorderProfileParamByName("vid.width",   (int)mCameraId, q);
  mVideoFrameHeight = mMediaProfiles->getCamcorderProfileParamByName("vid.height",  (int)mCameraId, q);
  mAudioCodec       = mMediaProfiles->getCamcorderProfileParamByName("aud.codec",   (int)mCameraId, q);
  mAudioBitRate     = mMediaProfiles->getCamcorderProfileParamByName("aud.bps",     (int)mCameraId, q);
  mAudioSampleRate  = mMediaProfiles->getCamcorderProfileParamByName("aud.hz",      (int)mCameraId, q);
  mAudioChannels    = mMediaProfiles->getCamcorderProfileParamByName("aud.ch",      (int)mCameraId, q);
  
  DoPullParameters(nsnull);

  // set params with new values
  const size_t SIZE = 256;
  char buffer[SIZE];
  // TODO: Ignore the width and height settings from app, just use the one in profile
  // eventually, will try to choose a profile which respects the settings from app
  mParams.setPreviewSize(mVideoFrameWidth, mVideoFrameHeight);
  mParams.setPreviewFrameRate(mVideoFrameRate);
  snprintf(buffer, SIZE, "%dx%d",mVideoFrameWidth,mVideoFrameHeight);
  // TODO: "record-size" is probably deprecated in later ICS
  // might need to set "video-size" instead of "record-size"
  mParams.set("record-size",buffer);
  // this is picture during video, for now set the picture size same as video dimensions
  // ideally we should make sure it matches the supported picture sizes
  mParams.setPictureSize(mVideoFrameWidth, mVideoFrameHeight);
  
  DoPushParameters(nsnull);
  return NS_OK;
}

nsresult
nsGonkCameraControl::DoToggleMode(ToggleModeTask *aToggleMode)
{
  nsresult result;
  nsCOMPtr<CameraPreview> preview;  // FIXME

  // stop preview
  mPreview->Stop();

  // setup video mode
  if ((result = SetupVideoMode()) != NS_OK) {
    goto bailout;
  }
  
  // restart preview
  preview = new GonkCameraPreview(mHwHandle, mVideoFrameWidth, mVideoFrameHeight);
  if (!preview) {
    result = NS_ERROR_OUT_OF_MEMORY;
    goto bailout;
  }
  mPreview = preview;

  // dispatch success
  if (NS_FAILED(NS_DispatchToMainThread(new ToggleModeResult(preview.get(), aToggleMode->mOnSuccessCb)))) {
    NS_WARNING("Failed to dispatch SwitchToVideoMode() onSuccess callback to main thread!");
  }
  return NS_OK;

bailout:
  if (NS_FAILED(NS_DispatchToMainThread(new CameraErrorResult(aToggleMode->mOnErrorCb, NS_LITERAL_STRING("OUT_OF_MEMORY"))))) {
    NS_WARNING("Failed to dispatch SwitchToVideoMode() onError callback to main thread!");
  }
  return result;
}

static int
GetFileNameWithDate(char *outstr, int maxlen)
{
  time_t t;
  struct tm *tmp;
  int actualLen;

  t = time(NULL);
  tmp = localtime(&t);
  if (tmp == NULL) {
    DOM_CAMERA_LOGE("localtime() failed\n");
    return 0;
  }

  if ((actualLen = strftime(outstr, maxlen, "video_%F__%H-%M-%S", tmp)) == 0) {
    DOM_CAMERA_LOGE("strftime() failed\n");
    return 0;
  }

  return actualLen;
}

static const char *
GetFileExtension(int nVideoFileFormat)
{
  if (nVideoFileFormat == OUTPUT_FORMAT_MPEG_4) {
    return ".mp4";
  }
  return ".3gp";
}

static int 
CreateVideoFile(nsAString& aVideoFile, int nVideoFileFormat)
{
  nsCOMPtr<nsIFile> f;
  nsCOMPtr<nsIDOMFile> tempFile;
  char fileName[MAX_VIDEO_FILE_NAME_LEN];
  char fileName2[MAX_VIDEO_FILE_NAME_LEN];
  char *videoFileAbsPath;
  struct stat buffer;
  int result;

  if (GetFileNameWithDate(fileName, MAX_VIDEO_FILE_NAME_LEN) == 0) {
    DOM_CAMERA_LOGW("Failed to get file name based to date, using default name: %s\n", DEFAULT_VIDEO_FILE_NAME);
    strncpy(fileName, DEFAULT_VIDEO_FILE_NAME, MAX_VIDEO_FILE_NAME_LEN);
  }

  // add the extension
  strncat(fileName, GetFileExtension(nVideoFileFormat), MAX_VIDEO_FILE_NAME_LEN);

  DOM_CAMERA_LOGI("Video File Name: \"%s\" \n", fileName);

  // check if the video storage dir exists
  result = stat(VIDEO_STORAGE_DIR, &buffer);
  if (0 == result) {
    snprintf(fileName2, MAX_VIDEO_FILE_NAME_LEN, VIDEO_STORAGE_DIR"/%s", fileName);
    videoFileAbsPath = fileName2;
  } else {
    DOM_CAMERA_LOGW("%s stat failed with error: %d:%s\n",VIDEO_STORAGE_DIR,errno,strerror(errno));
#if DEFAULT_VIDEO_STORAGE_TO_TEMP_DIR
    nsCAutoString filePath;
    DOM_CAMERA_LOGI("Attempting to use temp dir to store recorded file\n");
    // default to temp dir
    NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(f));
    if (!f) {
      DOM_CAMERA_LOGE("Failed to get temp directory path\n");
      return -1;
    }
    if (NS_FAILED(f->AppendNative(nsDependentCString(fileName)))) {
      DOM_CAMERA_LOGE("Failed to append file name to temp directory\n");
      return -1;
    }
    f->GetNativePath(filePath);
    videoFileAbsPath = (char*)filePath.get();
#else
    return -1;
#endif
  }

  DOM_CAMERA_LOGI("Opening video file: %s\n",videoFileAbsPath);
  int fd = open(videoFileAbsPath, O_RDWR | O_CREAT, 0744);
  if (fd < 0) {
    DOM_CAMERA_LOGE("Couldn't create file %s with error %d:%s\n",videoFileAbsPath,errno,strerror(errno));
  }
  
  aVideoFile.AssignASCII(fileName);
  return fd;
}

#ifndef CHECK_SETARG
#define CHECK_SETARG(x) {if(x) {DOM_CAMERA_LOGE(#x " failed\n"); return NS_ERROR_INVALID_ARG;}}
#endif

nsresult
nsGonkCameraControl::SetupRecording()
{
  const size_t SIZE = 256;
  char buffer[SIZE];

  mRecorder = new GonkRecorder();
  CHECK_SETARG(mRecorder->init());

  // set all the params
  CHECK_SETARG(mRecorder->setCameraHandle((int32_t)mHwHandle));
  CHECK_SETARG(mRecorder->setAudioSource(AUDIO_SOURCE_CAMCORDER));
  CHECK_SETARG(mRecorder->setVideoSource(VIDEO_SOURCE_CAMERA));
  CHECK_SETARG(mRecorder->setOutputFormat((output_format)mVideoFileFormat));
  CHECK_SETARG(mRecorder->setVideoFrameRate(mVideoFrameRate));
  CHECK_SETARG(mRecorder->setVideoSize(mVideoFrameWidth,mVideoFrameHeight));
  snprintf(buffer, SIZE, "video-param-encoding-bitrate=%d",mVideoBitRate);
  CHECK_SETARG(mRecorder->setParameters(String8(buffer)));
  CHECK_SETARG(mRecorder->setVideoEncoder((video_encoder)mVideoCodec));
  snprintf(buffer, SIZE, "audio-param-encoding-bitrate=%d",mAudioBitRate);
  CHECK_SETARG(mRecorder->setParameters(String8(buffer)));
  snprintf(buffer, SIZE, "audio-param-number-of-channels=%d",mAudioChannels);
  CHECK_SETARG(mRecorder->setParameters(String8(buffer)));
  snprintf(buffer, SIZE, "audio-param-sampling-rate=%d",mAudioSampleRate);
  CHECK_SETARG(mRecorder->setParameters(String8(buffer)));
  CHECK_SETARG(mRecorder->setAudioEncoder((audio_encoder)mAudioCodec));
  // TODO: For now there is no limit on recording duration
  CHECK_SETARG(mRecorder->setParameters(String8("max-duration=-1")));
  // TODO: For now there is no limit on file size
  CHECK_SETARG(mRecorder->setParameters(String8("max-filesize=-1")));
  snprintf(buffer, SIZE, "video-param-rotation-angle-degrees=%d",mVideoRotation);
  CHECK_SETARG(mRecorder->setParameters(String8(buffer)));

  // create video file
  int fd = CreateVideoFile(mVideoFile,mVideoFileFormat);
  if (fd < 0) {
    return NS_ERROR_FAILURE;
  }
  CHECK_SETARG(mRecorder->setOutputFile(fd,0,0));
  CHECK_SETARG(mRecorder->prepare());
  return NS_OK;
}

nsresult
nsGonkCameraControl::DoStartRecording(StartRecordingTask *aStartRecording)
{
  // do something with the options
  mVideoRotation = aStartRecording->mRotation;
  mVideoWidth = aStartRecording->mWidth;
  mVideoHeight = aStartRecording->mHeight;

  mStartRecordingOnSuccessCb = aStartRecording->mOnSuccessCb;
  mStartRecordingOnErrorCb = aStartRecording->mOnErrorCb;

  if (SetupRecording() != NS_OK) {
    return NS_ERROR_FAILURE;
  }

  if (mRecorder->start() != OK) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
nsGonkCameraControl::DoStopRecording(StopRecordingTask *aStopRecording)
{
  mRecorder->stop();

  // dispatch the callbacks
  nsCOMPtr<nsIRunnable> resultRunnable = new StartRecordingResult(mVideoFile, mStartRecordingOnSuccessCb);
  if (NS_FAILED(NS_DispatchToMainThread(resultRunnable))) {
    NS_WARNING("Failed to dispatch to main thread!");
  }

  delete mRecorder;
  mRecorder = nsnull;
  return NS_OK;
}

void
nsGonkCameraControl::ReceiveFrame(PRUint8* aData, PRUint32 aLength)
{
  nsCOMPtr<CameraPreview> preview = mPreview;

  if (preview) {
    GonkCameraPreview *p = static_cast<GonkCameraPreview *>(preview.get());
    if (p) {
      p->ReceiveFrame(aData, aLength);
    } else {
      DOM_CAMERA_LOGE("%s:%d : weird pointer problem happened\n");
    }
  }
}

/*
  Gonk callback handlers.
*/
BEGIN_CAMERA_NAMESPACE

void
GonkCameraReceiveImage(nsGonkCameraControl* gc, PRUint8* aData, PRUint32 aLength)
{
  gc->TakePictureComplete(aData, aLength);
}

void
GonkCameraAutoFocusComplete(nsGonkCameraControl* gc, bool success)
{
  gc->AutoFocusComplete(success);
}

void
GonkCameraReceiveFrame(nsGonkCameraControl* gc, PRUint8* aData, PRUint32 aLength)
{
  gc->ReceiveFrame(aData, aLength);
}

END_CAMERA_NAMESPACE

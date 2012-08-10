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

#include "nsDebug.h"
#include "GonkCameraHwMgr.h"
#include "GonkNativeWindow.h"

#define DOM_CAMERA_DEBUG_REFS 1
#define DOM_CAMERA_LOG_LEVEL        3
#include "CameraCommon.h"

using namespace mozilla;

#if GIHM_TIMING_RECEIVEFRAME
#define INCLUDE_TIME_H                  1
#endif
#if GIHM_TIMING_OVERALL
#define INCLUDE_TIME_H                  1
#endif

#if INCLUDE_TIME_H
#include <time.h>

static __inline void timespecSubtract(struct timespec* a, struct timespec* b)
{
  // a = b - a
  if (b->tv_nsec < a->tv_nsec) {
    b->tv_nsec += 1000000000;
    b->tv_sec -= 1;
  }
  a->tv_nsec = b->tv_nsec - a->tv_nsec;
  a->tv_sec = b->tv_sec - a->tv_sec;
}
#endif

GonkCameraHardware::GonkCameraHardware(GonkCamera* aTarget, PRUint32 aCamera)
  : mCamera(aCamera)
  , mClosing(false)
  , mMonitor("GonkCameraHardware.Monitor")
  , mNumFrames(0)
  , mTarget(aTarget)
  , mInitialized(false)
{
  DOM_CAMERA_LOGI( "%s: this = %p (aTarget = %p)\n", __func__, (void* )this, (void* )aTarget );
  Init();
}

// Android data callback
void
GonkCameraHardware::DataCallback(int32_t aMsgType, const sp<IMemory> &aDataPtr, camera_frame_metadata_t* aMetadata, void* aUser)
{
  GonkCameraHardware* hw = GetHardware((PRUint32)aUser);
  if (!hw) {
    DOM_CAMERA_LOGW("%s:aUser = %d resolved to no camera hw\n", __func__, (PRUint32)aUser);
    return;
  }
  if (hw->mClosing) {
    return;
  }

  GonkCamera* camera = hw->mTarget;
  if (camera) {
    switch (aMsgType) {
      case CAMERA_MSG_PREVIEW_FRAME:
        ReceiveFrame(camera, (PRUint8*)aDataPtr->pointer(), aDataPtr->size());
        break;

      case CAMERA_MSG_COMPRESSED_IMAGE:
        ReceiveImage(camera, (PRUint8*)aDataPtr->pointer(), aDataPtr->size());
        break;

      default:
        DOM_CAMERA_LOGE("Unhandled data callback event %d\n", aMsgType);
        break;
    }
  } else {
    DOM_CAMERA_LOGW("%s: hw = %p (camera = NULL)\n", __func__, hw);
  }
}

// Android notify callback
void
GonkCameraHardware::NotifyCallback(int32_t aMsgType, int32_t ext1, int32_t ext2, void* aUser)
{
  bool bSuccess;
  GonkCameraHardware* hw = GetHardware((PRUint32)aUser);
  if (!hw) {
    DOM_CAMERA_LOGW("%s:aUser = %d resolved to no camera hw\n", __func__, (PRUint32)aUser);
    return;
  }
  if (hw->mClosing) {
    return;
  }

  GonkCamera* camera = hw->mTarget;
  if (!camera) {
    return;
  }

  switch (aMsgType) {
    case CAMERA_MSG_FOCUS:
      if (ext1) {
        DOM_CAMERA_LOGI("Autofocus complete");
        bSuccess = true;
      } else {
        DOM_CAMERA_LOGW("Autofocus failed");
        bSuccess = false;
      }
      AutoFocusComplete(camera, bSuccess);
      break;

    case CAMERA_MSG_SHUTTER:
      DOM_CAMERA_LOGW("Shutter event not handled yet\n");
      break;

    default:
      DOM_CAMERA_LOGE("Unhandled notify callback event %d\n", aMsgType);
      break;
  }
}

void
GonkCameraHardware::Init()
{
  DOM_CAMERA_LOGI("%s: this = %p\n", __func__, (void* )this);

  if (hw_get_module(CAMERA_HARDWARE_MODULE_ID, (const hw_module_t**)&mModule) < 0) {
    return;
  }
  char cameraDeviceName[4];
  snprintf(cameraDeviceName, sizeof(cameraDeviceName), "%d", mCamera);
  mHardware = new CameraHardwareInterface(cameraDeviceName);
  if (mHardware->initialize(&mModule->common) != OK) {
    mHardware.clear();
    return;
  }

  mWindow = new android::GonkNativeWindow();

  if (sHwHandle == 0) {
    sHwHandle = 1;  // don't use 0
  }
  mHardware->setCallbacks(GonkCameraHardware::NotifyCallback, GonkCameraHardware::DataCallback, NULL, (void*)sHwHandle);
  mHardware->setPreviewWindow(mWindow);
  mInitialized = true;
}

GonkCameraHardware::~GonkCameraHardware()
{
  DOM_CAMERA_LOGI( "%s:%d : this = %p\n", __func__, __LINE__, (void*)this );
  sHw = nullptr;
}

GonkCameraHardware* GonkCameraHardware::sHw         = nullptr;
PRUint32            GonkCameraHardware::sHwHandle   = 0;

void
GonkCameraHardware::ReleaseHandle(PRUint32 aHwHandle)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  DOM_CAMERA_LOGI("%s: aHwHandle = %d, hw = %p (sHwHandle = %d)\n", __func__, aHwHandle, (void*)hw, sHwHandle);
  if (!hw) {
    return;
  }

  DOM_CAMERA_LOGI("%s: before: sHwHandle = %d\n", __func__, sHwHandle);
  sHwHandle += 1; // invalidate old handles before deleting
  hw->mClosing = true;
  hw->mHardware->disableMsgType(CAMERA_MSG_ALL_MSGS);
  hw->mHardware->stopPreview();
  hw->mHardware->release();
  DOM_CAMERA_LOGI("%s: after: sHwHandle = %d\n", __func__, sHwHandle);
  delete hw;     // destroy the camera hardware instance
}

PRUint32
GonkCameraHardware::GetHandle(GonkCamera* aTarget, PRUint32 aCamera)
{
  ReleaseHandle(sHwHandle);

  sHw = new GonkCameraHardware(aTarget, aCamera);

  if (sHw->IsInitialized()) {
    return sHwHandle;
  }

  DOM_CAMERA_LOGE("failed to initialize camera hardware\n");
  delete sHw;
  sHw = nullptr;
  return 0;
}

int
GonkCameraHardware::AutoFocus(PRUint32 aHwHandle)
{
  DOM_CAMERA_LOGI("%s: aHwHandle = %d\n", __func__, aHwHandle);
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (!hw) {
    return DEAD_OBJECT;
  }

  hw->mHardware->enableMsgType(CAMERA_MSG_FOCUS);
  return hw->mHardware->autoFocus();
}

void
GonkCameraHardware::CancelAutoFocus(PRUint32 aHwHandle)
{
  DOM_CAMERA_LOGI("%s: aHwHandle = %d\n", __func__, aHwHandle);
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (hw) {
    hw->mHardware->cancelAutoFocus();
  }
}

int
GonkCameraHardware::TakePicture(PRUint32 aHwHandle)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (!hw) {
    return DEAD_OBJECT;
  }

  hw->mHardware->enableMsgType(CAMERA_MSG_COMPRESSED_IMAGE);
  return hw->mHardware->takePicture();
}

void
GonkCameraHardware::CancelTakePicture(PRUint32 aHwHandle)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (hw) {
    hw->mHardware->cancelPicture();
  }
}

int
GonkCameraHardware::PushParameters(PRUint32 aHwHandle, const CameraParameters& aParams)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (!hw) {
    return DEAD_OBJECT;
  }

  return hw->mHardware->setParameters(aParams);
}

void
GonkCameraHardware::PullParameters(PRUint32 aHwHandle, CameraParameters& aParams)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (hw) {
    aParams = hw->mHardware->getParameters();
  }
}

int
GonkCameraHardware::StartPreview()
{
  mHardware->enableMsgType(CAMERA_MSG_PREVIEW_FRAME);
  return mHardware->startPreview();
}

int
GonkCameraHardware::StartPreview(PRUint32 aHwHandle)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  DOM_CAMERA_LOGI("%s:%d : aHwHandle = %d, hw = %p\n", __func__, __LINE__, aHwHandle, hw);
  if (!hw) {
    return DEAD_OBJECT;
  }

  return hw->StartPreview();
}

void
GonkCameraHardware::StopPreview(PRUint32 aHwHandle)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (hw) {
    hw->mHardware->stopPreview();
  }
}

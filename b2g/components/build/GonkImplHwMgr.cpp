/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "nsDebug.h"
#include "GonkImplHwMgr.h"


// config
#define GIHM_STUB_NATIVEWINDOW      1
#define GIHM_LOG_LEVEL              2

#define GIHM_LOG( l, ... )       \
  do {                                  \
    if ( GIHM_LOG_LEVEL >= (l) ) {  \
      printf_stderr (__VA_ARGS__); \
    }                                   \
  } while (0)

#define GIHM_LOGA( ... )        GIHM_LOG( 0, __VA_ARGS__ )

#if GIHM_LOG_LEVEL
enum {
  LOG_NOTHING,
  LOG_ERROR,
  LOG_WARNING,
  LOG_INFO
};

#define GIHM_LOGI( ... )        GIHM_LOG( LOG_INFO,  __VA_ARGS__ )
#define GIHM_LOGW( ... )        GIHM_LOG( LOG_WARNING, __VA_ARGS__ )
#define GIHM_LOGE( ... )        GIHM_LOG( LOG_ERROR, __VA_ARGS__ )
#else
#define GIHM_LOGI( ... )
#define GIHM_LOGW( ... )
#define GIHM_LOGE( ... )
#endif

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


#if GIHM_STUB_NATIVEWINDOW
// to be replaced by GonkNativeWindow.(h|cpp)
int WindowSetSwapInterval(struct ANativeWindow* window, int interval) {
  return 0;
}

int WindowDequeueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer) {
  return 0;
}

int WindowLockBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
  return 0;
}

int WindowQueueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
  return 0;
}

int WindowQuery(const struct ANativeWindow* window, int what, int* value) {
  return 0;
}

int WindowPerform(struct ANativeWindow* window, int operation, ... ) {
  return 0;
}

int WindowCancelBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
  return 0;
}

void WindowIncRef(struct android_native_base_t* base) {

}

void WindowDecRef(struct android_native_base_t* base) {

}
#else
#include "CameraNativeWindow.h"
#endif

GonkCameraHardware::GonkCameraHardware(GonkCamera* aTarget, PRUint32 aCamera, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aFps) :
  mCamera(aCamera), mWidth(aWidth), mHeight(aHeight), mFps(aFps),
  mIs420p(false), mClosing(false), mMonitor("GonkCameraHardware.Monitor"),
  mNumFrames(0), mTarget(aTarget)
{
  GIHM_LOGI( "%s: this = %p (aTarget = %p)\n", __func__, (void*)this, (void*)aTarget );
  init();
}

// Android data callback
void
GonkCameraHardware::DataCallback(int32_t aMsgType, const sp<IMemory> &aDataPtr, camera_frame_metadata_t *aMetadata, void* aUser)
{
  GonkCameraHardware* hw = getCameraHardware((PRUint32)aUser);
  if (!hw) {
    GIHM_LOGW("%s:aUser = %d resolved to no camera hw\n", __func__, (PRUint32)aUser);
    return;
  }
  if (hw->mClosing) {
    return;
  }

  GonkCamera* camera = hw->mTarget;
  if (camera) {
    switch (aMsgType) {
      case CAMERA_MSG_PREVIEW_FRAME:
        GonkCameraReceiveFrame(camera, (PRUint8*)aDataPtr->pointer(), aDataPtr->size());
        break;

      case CAMERA_MSG_COMPRESSED_IMAGE:
        GonkCameraReceiveImage(camera, (PRUint8*)aDataPtr->pointer(), aDataPtr->size());
        break;

      default:
        GIHM_LOGE("Unhandled data callback event %d\n", aMsgType);
        break;
    }
  } else {
    GIHM_LOGW( "%s: hw = %p (camera = NULL)\n", __func__, hw );
  }
}

// Android notify callback
void
GonkCameraHardware::NotifyCallback(int32_t aMsgType, int32_t ext1, int32_t ext2, void* aUser)
{
  bool bSuccess;
  GonkCameraHardware* hw = getCameraHardware((PRUint32)aUser);
  if (!hw) {
    GIHM_LOGW("%s:aUser = %d resolved to no camera hw\n", __func__, (PRUint32)aUser);
    return;
  }
  if (hw->mClosing) {
    return;
  }

  GonkCamera* camera = hw->mTarget;
  if (camera) {
    switch (aMsgType) {
      case CAMERA_MSG_FOCUS:
        if (ext1) {
          GIHM_LOGI("Autofocus complete");
          bSuccess = true;
        } else {
          GIHM_LOGW("Autofocus failed");
          bSuccess = false;
        }
        GonkCameraAutoFocusComplete(camera, bSuccess);
        break;

      default:
        GIHM_LOGE("Unhandled notify callback event %d\n", aMsgType);
        break;
    }
  }
}

void
GonkCameraHardware::init()
{
  GIHM_LOGI( "%s: this = %p\n", __func__, (void*)this );

  if (hw_get_module(CAMERA_HARDWARE_MODULE_ID,
            (const hw_module_t **)&mModule) < 0) {
    return;
  }
  char cameraDeviceName[4];
  snprintf(cameraDeviceName, sizeof(cameraDeviceName), "%d", mCamera);
  mHardware = new CameraHardwareInterface(cameraDeviceName);
  if (mHardware->initialize(&mModule->common) != OK) {
    mHardware.clear();
    return;
  }

#if GIHM_STUB_NATIVEWINDOW
  ANativeWindow* window = new ANativeWindow();
  window->common.incRef = WindowIncRef;
  window->common.decRef = WindowDecRef;
  mWindow = window;
  mWindow->setSwapInterval = WindowSetSwapInterval;
  mWindow->dequeueBuffer = WindowDequeueBuffer;
  mWindow->lockBuffer = WindowLockBuffer;
  mWindow->queueBuffer = WindowQueueBuffer;
  mWindow->query = WindowQuery;
  mWindow->perform = WindowPerform;
  mWindow->cancelBuffer = WindowCancelBuffer;
#else
  mWindow = new android::CameraNativeWindow();
#endif

  if (sHwHandle == 0) {
    sHwHandle = 1;  // don't use 0
  }
  mHardware->setCallbacks(GonkCameraHardware::NotifyCallback, GonkCameraHardware::DataCallback, NULL, (void*)sHwHandle);

  mHardware->enableMsgType(CAMERA_MSG_PREVIEW_FRAME);

  mParams = mHardware->getParameters();

  GIHM_LOGA("Preview formats: %s\n", mParams.get(mParams.KEY_SUPPORTED_PREVIEW_FORMATS));

  Vector<Size> previewSizes;
  mParams.getSupportedPreviewSizes(previewSizes);

  // find the available preview size closest to the requested size.
  PRUint32 minSizeDelta = PR_UINT32_MAX;
  PRUint32 bestWidth = mWidth;
  PRUint32 bestHeight = mHeight;
  for (PRUint32 i = 0; i < previewSizes.size(); i++) {
    Size size = previewSizes[i];
    PRUint32 delta = abs((long int)(size.width * size.height - mWidth * mHeight));
    if (delta < minSizeDelta) {
      minSizeDelta = delta;
      bestWidth = size.width;
      bestHeight = size.height;
    }
  }
  mWidth = bestWidth;
  mHeight = bestHeight;
  mParams.setPreviewSize(mWidth, mHeight);

  // try to set preferred image format and frame rate
  const char* const PREVIEW_FORMAT = "yuv420p";
  mParams.setPreviewFormat(PREVIEW_FORMAT);
  mParams.setPreviewFrameRate(mFps);
  mHardware->setParameters(mParams);

  // check that our settings stuck
  mParams = mHardware->getParameters();
  mIs420p = strcmp(mParams.getPreviewFormat(), PREVIEW_FORMAT) == 0;
  if (!mIs420p) {
    GIHM_LOGA("Camera ignored our request for '%s' format, will have to convert\n", PREVIEW_FORMAT);
  }
  // Check the frame rate and log if the camera ignored our setting
  PRUint32 fps = mParams.getPreviewFrameRate();
  if (fps != mFps) {
    GIHM_LOGA("We asked for %d fps but camera returned %d fps, using it", mFps, fps);
    mFps = fps;
  }

  mHardware->setPreviewWindow(mWindow);
}

GonkCameraHardware::~GonkCameraHardware()
{
  GIHM_LOGI( "XxXxX %s: this = %p\n", __func__, (void*)this );
  sHw = nsnull;
}

GonkCameraHardware* GonkCameraHardware::sHw         = nsnull;
PRUint32            GonkCameraHardware::sHwHandle   = 0;

void
GonkCameraHardware::releaseCameraHardwareHandle(PRUint32 aHwHandle)
{
  GonkCameraHardware* hw = getCameraHardware(aHwHandle);
  GIHM_LOGI("%s: aHwHandle = %d, hw = %p (sHwHandle = %d)\n", __func__, aHwHandle, (void*)hw, sHwHandle);

  if (hw) {
    GIHM_LOGI("%s: before: sHwHandle = %d\n", __func__, sHwHandle);
    sHwHandle += 1; // invalidate old handles before deleting
    hw->mClosing = true;
    hw->mHardware->disableMsgType(CAMERA_MSG_ALL_MSGS);
    hw->mHardware->stopPreview();
    hw->mHardware->release();
    GIHM_LOGI("%s: after: sHwHandle = %d\n", __func__, sHwHandle);
    delete hw;     // destroy the camera hardware instance
  }
}

PRUint32
GonkCameraHardware::getCameraHardwareHandle(GonkCamera* aTarget, PRUint32 aCamera, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aFps)
{
  releaseCameraHardwareHandle(sHwHandle);

  sHw = new GonkCameraHardware(aTarget, aCamera, aWidth, aHeight, aFps);
  return sHwHandle;
}

PRUint32
GonkCameraHardware::getCameraHardwareFps(PRUint32 aHwHandle)
{
  GonkCameraHardware* hw = getCameraHardware(aHwHandle);
  if (hw) {
    return hw->mFps;
  } else {
    return 0;
  }
}

void
GonkCameraHardware::getCameraHardwareSize(PRUint32 aHwHandle, PRUint32* aWidth, PRUint32* aHeight)
{
  GonkCameraHardware* hw = getCameraHardware(aHwHandle);
  if (hw) {
    *aWidth = hw->mWidth;
    *aHeight = hw->mHeight;
  } else {
    *aWidth = 0;
    *aHeight = 0;
  }
}

int
GonkCameraHardware::doCameraHardwareAutofocus(PRUint32 aHwHandle)
{
  GIHM_LOGI("%s: aHwHandle = %d\n", __func__, aHwHandle);
  GonkCameraHardware* hw = getCameraHardware(aHwHandle);
  if (hw) {
    hw->mHardware->enableMsgType(CAMERA_MSG_FOCUS);
    return hw->mHardware->autoFocus();
  } else {
    return !OK;
  }
}

int
GonkCameraHardware::doCameraHardwareTakePicture(PRUint32 aHwHandle)
{
  GonkCameraHardware* hw = getCameraHardware(aHwHandle);
  if (hw) {
    hw->mHardware->enableMsgType(CAMERA_MSG_COMPRESSED_IMAGE);
    return hw->mHardware->takePicture();
  } else {
    return !OK;
  }
}

const char*
GonkCameraHardware::getCameraHardwareParameter(PRUint32 aHwHandle, const char* key)
{
  GonkCameraHardware* hw = getCameraHardware(aHwHandle);
  if (hw) {
    return hw->mParams.get(key);
  } else {
    return nsnull;
  }
}

void
GonkCameraHardware::setCameraHardwareParameter(PRUint32 aHwHandle, const char* key, const char* value)
{
  GonkCameraHardware* hw = getCameraHardware(aHwHandle);
  if (hw) {
    hw->mParams.set(key, value);
    hw->mHardware->setParameters(hw->mParams);
  }
}

int
GonkCameraHardware::doCameraHardwareStartPreview(PRUint32 aHwHandle)
{
  GonkCameraHardware* hw = getCameraHardware(aHwHandle);
  if (hw) {
    return hw->mHardware->startPreview();
  } else {
    return !OK;
  }
}

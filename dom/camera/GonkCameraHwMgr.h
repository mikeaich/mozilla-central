/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_GONKCAMERAHWMGR_H
#define DOM_CAMERA_GONKCAMERAHWMGR_H


#include "libcameraservice/CameraHardwareInterface.h"
#include "binder/IMemory.h"
#include "mozilla/ReentrantMonitor.h"

#include "GonkCameraControl.h"
#include "CameraCommon.h"

// config
#define GIHM_TIMING_RECEIVEFRAME    0
#define GIHM_TIMING_OVERALL         1


using namespace mozilla;
using namespace android;

BEGIN_CAMERA_NAMESPACE

typedef class nsGonkCameraControl GonkCamera;

class GonkCameraHardware
{
protected:
  GonkCameraHardware(GonkCamera* aTarget, PRUint32 aCamera);
  ~GonkCameraHardware();
  void init();

  static void                   DataCallback(int32_t aMsgType, const sp<IMemory> &aDataPtr, camera_frame_metadata_t *aMetadata, void* aUser);
  static void                   NotifyCallback(int32_t aMsgType, int32_t ext1, int32_t ext2, void* aUser);

public:
  static void                   ReleaseHandle(PRUint32 aHwHandle);
  static PRUint32               GetHandle(GonkCamera* aTarget, PRUint32 aCamera);
  static PRUint32               GetFps(PRUint32 aHwHandle);
  static void                   GetPreviewSize(PRUint32 aHwHandle, PRUint32* aWidth, PRUint32* aHeight);
  static void                   SetPreviewSize(PRUint32 aHwHandle, PRUint32 aWidth, PRUint32 aHeight);
  static int                    AutoFocus(PRUint32 aHwHandle);
  static void                   CancelAutoFocus(PRUint32 aHwHandle);
  static int                    TakePicture(PRUint32 aHwHandle);
  static void                   CancelTakePicture(PRUint32 aHwHandle);
  static int                    StartPreview(PRUint32 aHwHandle);
  static void                   StopPreview(PRUint32 aHwHandle);
  static int                    PushParameters(PRUint32 aHwHandle, const CameraParameters& aParams);
  static void                   PullParameters(PRUint32 aHwHandle, CameraParameters& aParams);

  enum {
    PREVIEW_FORMAT_UNKNOWN,
    PREVIEW_FORMAT_YUV420P,
    PREVIEW_FORMAT_YUV420SP
  };
  static PRUint32               GetPreviewFormat(PRUint32 aHwHandle);

protected:
  static GonkCameraHardware*    sHw;
  static PRUint32               sHwHandle;

  static GonkCameraHardware*    getCameraHardware(PRUint32 aHwHandle)
  {
    if (aHwHandle == sHwHandle) {
      /*
        In the initial case, sHw will be null and sHwHandle will be 0,
        so even if this function is called with aHwHandle = 0, the
        result will still be null.
      */
      return sHw;
    } else {
      return nsnull;
    }
  }

  /* instance wrappers to make member function access easier */
  void setPreviewSize(PRUint32 aWidth, PRUint32 aHeight);
  int startPreview();

  PRUint32                      mCamera;
  PRUint32                      mWidth;
  PRUint32                      mHeight;
  PRUint32                      mFps;
  PRUint32                      mPreviewFormat;
  bool                          mClosing;
  mozilla::ReentrantMonitor     mMonitor;
  PRUint32                      mNumFrames;
  sp<CameraHardwareInterface>   mHardware;
  GonkCamera*                   mTarget;
  camera_module_t*              mModule;
  sp<ANativeWindow>             mWindow;
  CameraParameters              mParams;
#if GIHM_TIMING_OVERALL
  struct timespec               mStart;
  struct timespec               mAutoFocusStart;
#endif
  bool                          mInitialized;

  bool initialized()
  {
    return mInitialized;
  }

private:
  GonkCameraHardware(const GonkCameraHardware&);
  GonkCameraHardware& operator=(const GonkCameraHardware&);
};

END_CAMERA_NAMESPACE


#endif // GONK_IMPL_HW_MGR_H

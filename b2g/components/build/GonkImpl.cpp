/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "nsCOMPtr.h"
#include "nsDOMMediaStream.h"
#include "cameracontrol.h"
#include "CameraImpl.h"
#include "MediaStreamGraph.h"
#include "BaseCameraStream.h"
#include "VideoUtils.h"
#include "StreamBuffer.h"
#include "nsDOMFile.h"
#include "nsIRunnable.h"
#include "GonkImplHwMgr.h"
#include "GonkImpl.h"


#define GONKIMPL_LOG_LEVEL      3

#define GONKIMPL_LOG( l, ... )       \
  do {                                  \
    if ( GONKIMPL_LOG_LEVEL >= (l) ) {  \
      printf_stderr (__VA_ARGS__); \
    }                                   \
  } while (0)

#define GONKIMPL_LOGA( ... )        GONKIMPL_LOG( 0, __VA_ARGS__ )

#if GONKIMPL_LOG_LEVEL
enum {
  LOG_NOTHING,
  LOG_ERROR,
  LOG_WARNING,
  LOG_INFO
};

#define GONKIMPL_LOGI( ... )        GONKIMPL_LOG( LOG_INFO,  __VA_ARGS__ )
#define GONKIMPL_LOGW( ... )        GONKIMPL_LOG( LOG_WARNING, __VA_ARGS__ )
#define GONKIMPL_LOGE( ... )        GONKIMPL_LOG( LOG_ERROR, __VA_ARGS__ )
#else
#define GONKIMPL_LOGI( ... )
#define GONKIMPL_LOGW( ... )
#define GONKIMPL_LOGE( ... )
#endif

#define GONKIMPL_DEBUG_REFCNT   0

#if GONKIMPL_DEBUG_REFCNT
#define LOG_REFCNT( s )                         \
  do {                                          \
    if (s) {                                    \
      gilog_refcnt( __func__, __LINE__, (s) );  \
    }                                           \
  } while( 0 )

static void gilog_refcnt( const char* f, int l, nsIDOMMediaStream* s )
{
  printf_stderr( "%s:%d refcnt = %d\n", f, l, NS_ADDREF(s) - 1 );
  NS_RELEASE( s );
}
#else
#define LOG_REFCNT( s )
#endif


using namespace mozilla;
using namespace mozilla::layers;


static const TrackID TRACK_AUDIO = 1;
static const TrackID TRACK_VIDEO = 2;


class CameraAutoFocusResultTask : public nsRunnable
{
public:
  CameraAutoFocusResultTask(nsICameraAutoFocusCallback *onSuccess)
    : mOnSuccessCb(onSuccess)
  {
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent();
    }
    return NS_OK;
  }

  nsICameraAutoFocusCallback *mOnSuccessCb;
};

class CameraTakePictureResultTask : public nsRunnable
{
public:
  CameraTakePictureResultTask(nsIDOMBlob *pic, nsICameraTakePictureCallback *onSuccess)
    : mDOMBlob(pic), mOnSuccessCb(onSuccess)
  {
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mDOMBlob);
    }

    return NS_OK;
  }

  nsIDOMBlob *mDOMBlob;
  nsICameraTakePictureCallback *mOnSuccessCb;
};

class CameraPreviewStreamResultTask : public nsRunnable
{
public:
  CameraPreviewStreamResultTask(nsIDOMMediaStream *stream, nsICameraPreviewStreamCallback *onSuccess)
    : mStream(stream), mOnSuccessCb(onSuccess)
  {
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mStream);
    }

    return NS_OK;
  }

  nsIDOMMediaStream *mStream;
  nsICameraPreviewStreamCallback *mOnSuccessCb;
};

class CameraStartRecordingResultTask : public nsRunnable
{
public:
  CameraStartRecordingResultTask(nsIDOMMediaStream *stream, nsICameraStartRecordingCallback *onSuccess)
    : mStream(stream), mOnSuccessCb(onSuccess)
  {
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mStream);
    }

    return NS_OK;
  }

  nsIDOMMediaStream *mStream;
  nsICameraStartRecordingCallback *mOnSuccessCb;
};

class CameraShutterCallbackTask : public nsRunnable
{
public:
  CameraShutterCallbackTask(nsICameraShutterCallback *onShutter)
    : mOnShutterCb(onShutter)
  {
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnShutterCb) {
      mOnShutterCb->HandleEvent();
    }

    return NS_OK;
  }

  nsICameraShutterCallback *mOnShutterCb;
};

class CameraErrorResultTask : public nsRunnable
{
public:
  CameraErrorResultTask(nsICameraErrorCallback *onError, nsAString & error)
    : mOnErrorCb(onError), mError(error)
  {
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnErrorCb) {
      mOnErrorCb->HandleEvent(mError);
    }

    return NS_OK;
  }

  nsICameraErrorCallback *mOnErrorCb;
  nsAString & mError;
};

class GonkCameraCapabilities : public nsICameraCapabilities
{
public:
  
};

class GonkCamera : public nsICameraControl
                 , public nsDOMMediaStream
                 , public BaseCameraStream
                 , public MediaStreamListener
{
public:
  NS_DECL_ISUPPORTS

  GonkCamera(PRUint32 aCamera, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aFps);
  ~GonkCamera();

  /* nsIDOMDOMRequest autoFocus (); */
  NS_IMETHODIMP
  AutoFocus(nsICameraAutoFocusCallback *onSuccess, nsICameraErrorCallback *onError);

  /* nsIDOMDOMRequest takePicture (); */
  NS_IMETHODIMP
  TakePicture(nsICameraTakePictureCallback *onSuccess, nsICameraErrorCallback *onError);

  /* [implicit_jscontext] jsval getParameter (in DOMString name); */
  NS_IMETHODIMP
  GetParameter(const nsAString & name, JSContext* cx, JS::Value *_retval NS_OUTPARAM);

#if 0
  /* [implicit_jscontext] void setParameter (in DOMString name, in jsval value); */
  NS_IMETHODIMP
  SetParameter(const nsAString & name, const JS::Value & value, JSContext* cx);

  NS_IMETHODIMP
  GetCurrentTime(double* aCurrentTime) {
    return nsDOMMediaStream::GetCurrentTime(aCurrentTime);
  }
#endif

  /* camera driver callbacks */
  void
  ReceiveImage(PRUint8* aData, PRUint32 aLength);

  void
  AutoFocusComplete(bool success);

  void
  ReceiveFrame(PRUint8* aData, PRUint32 aLength);
  
  /* MediaStreamListener callbacks */
  void
  NotifyConsumptionChanged(MediaStreamGraph* aGraph, Consumption aConsuming);
  
public:
  nsCOMPtr<nsICameraAutoFocusCallback>      mAutoFocusOnSuccessCb;
  nsCOMPtr<nsICameraTakePictureCallback>    mTakePictureOnSuccessCb;
  nsCOMPtr<nsICameraPreviewStreamCallback>  mPreviewStreamOnSuccessCb;
  nsCOMPtr<nsICameraStartRecordingCallback> mStartRecordingOnSuccessCb;
  nsCOMPtr<nsICameraShutterCallback>        mOnShutterCb;
  nsCOMPtr<nsICameraErrorCallback>          mOnErrorCb;

protected:
  enum State {
    HW_STATE_CLOSED,
    HW_STATE_PREVIEW,
    HW_STATE_CLOSING,
    HW_STATE_TAKING_PICTURE,
    HW_STATE_PREVIEW_PAUSED
  };

  State                 mState;
  PRUint32              mCamera;
  PRUint32              mWidth;
  PRUint32              mHeight;
  PRUint32              mFps;
  bool                  mIs420p;
  bool                  mPreviewStarted;
  SourceMediaStream*    mInput;
  nsRefPtr<mozilla::layers::ImageContainer> mImageContainer;
  VideoSegment          mVideoSegment;
  mozilla::ReentrantMonitor mMonitor;
  PRUint32              mNumFrames;
  PRUint32              mHwHandle;
};

GonkCamera::GonkCamera(PRUint32 aCamera, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aFps) :
  nsDOMMediaStream(), mState(HW_STATE_CLOSED), mCamera(aCamera), mWidth(aWidth),
  mHeight(aHeight), mFps(aFps), mIs420p(false), mPreviewStarted(false),
  mMonitor("GonkCamera.Monitor"), mNumFrames(0)
{
  GONKIMPL_LOGI("%s: this = %p\n", __func__, (void*)this);

  mImageContainer = LayerManager::CreateImageContainer();
  MediaStreamGraph* gm = MediaStreamGraph::GetInstance();
  mStream = gm->CreateInputStream(this);
  mInput = GetStream()->AsSourceStream();
  mInput->AddListener(this);
  
  mHwHandle = GonkCameraHardware::getCameraHardwareHandle(this, mCamera, mWidth, mHeight, mFps);
}

GonkCamera::~GonkCamera()
{
  GONKIMPL_LOGI("%s: this = %p\n", __func__, (void*)this);

#if GONKIMPL_TIMING_OVERALL
  struct timespec end;
  double seconds;
  double fps;

  clock_gettime(CLOCK_MONOTONIC, &end);
  timespecSubtract(&mStart, &end); // mStart = end - mStart
  seconds = mStart.tv_nsec;
  seconds /= 1000000000;
  seconds += mStart.tv_sec;
  fps = mNumFrames / seconds;
  printf_stderr("==> %d preview frames over %g seconds, %g frames per second <==\n", mNumFrames, seconds, fps);
#endif

  mState = HW_STATE_CLOSING;
  GonkCameraHardware::releaseCameraHardwareHandle(mHwHandle);
  
  mInput->RemoveListener(this);
}

void
GonkCamera::NotifyConsumptionChanged(MediaStreamGraph* aGraph, Consumption aConsuming)
{
  const char* state;
  
  switch (aConsuming) {
    case NOT_CONSUMED:
      state = "not consuming";
      break;
    
    case CONSUMED:
      state = "consuming";
      break;
    
    default:
      state = "unknown";
      break;
  }
  
  printf_stderr("camera viewfinder is %s\n", state);
  
  switch (aConsuming) {
    case NOT_CONSUMED:
      GonkCameraHardware::doCameraHardwareStopPreview(mHwHandle);
      break;
    
    case CONSUMED:
      if (GonkCameraHardware::doCameraHardwareStartPreview(mHwHandle) == OK) {
        mState = HW_STATE_PREVIEW;
        mFps = GonkCameraHardware::getCameraHardwareFps(mHwHandle);
        GonkCameraHardware::getCameraHardwareSize(mHwHandle, &mWidth, &mHeight);
        mInput->AddTrack(TRACK_VIDEO, mFps, 0, new VideoSegment());
        mInput->AdvanceKnownTracksTime(MEDIA_TIME_MAX);
      } else {
        GONKIMPL_LOGE("%s: failed to start preview\n", __func__);
      }
      break;
  }
}

/* nsIDOMDOMRequest autofocus (); */
NS_IMETHODIMP
GonkCamera::AutoFocus(nsICameraAutoFocusCallback *onSuccess, nsICameraErrorCallback *onError)
{
#if GONKIMPL_TIMING_OVERALL
  clock_gettime(CLOCK_MONOTONIC, &mAutoFocusStart);
#endif

  ReentrantMonitorAutoEnter enter(mMonitor);
/*
  Will probably need to build the response runnable here, populate it
  with pointers to the callback functions, then attach it to the
  autofocus request--if possible.
  
  LOG_REFCNT(this);

  mAFOnSuccessCB = onsuccess;
  mAFOnErrorCB = onerror;

  if (GonkCameraHardware::doCameraHardwareAutofocus(mHwHandle) != OK) {
    return NS_ERROR_FAILURE;
  }

  LOG_REFCNT(this);
*/
  return NS_OK;
}

/* nsIDOMDOMRequest takePicture (); */
NS_IMETHODIMP
GonkCamera::TakePicture(nsICameraTakePictureCallback *onSuccess, nsICameraErrorCallback *onError)
{
  mState = HW_STATE_TAKING_PICTURE;
  ReentrantMonitorAutoEnter enter(mMonitor);
/*
  Will probably need to build the response runnable here, populate it
  with pointers to the callback functions, then attach it to the
  take picture request--if possible.
  
  LOG_REFCNT(this);

  mTPOnSuccessCB = onsuccess;
  mTPOnErrorCB = onerror;

  if (GonkCameraHardware::doCameraHardwareTakePicture(mHwHandle) != OK) {
    return NS_ERROR_FAILURE;
  }

  LOG_REFCNT(this);
*/
  return NS_OK;
}

#if 0
/* [implicit_jscontext] jsval getParameter (in DOMString name); */
NS_IMETHODIMP
GonkCamera::GetParameter(const nsAString & name, JSContext* cx, JS::Value *_retval NS_OUTPARAM)
{
  /*
    Example:

    char* s = ToNewCString( name );
    printf_stderr( "%s: name = '%s'\n", __func__, s );
    nsMemory::Free( s );
    *_retval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "Hello, world!"));
  */

  ReentrantMonitorAutoEnter enter(mMonitor);

  char* k = ToNewCString(name);
  const char* v = GonkCameraHardware::getCameraHardwareParameter(mHwHandle, k);
  bool found = true;

  GONKIMPL_LOGI("%s: key '%s' --> value '%s'\n", __func__, k, v);
  if (v) {
    *_retval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, v));
  } else {
    found = false;
  }

  nsMemory::Free(k);

  if (found) {
    return NS_OK;
  } else {
    return NS_ERROR_INVALID_ARG;
  }
}

/* [implicit_jscontext] void setParameter (in DOMString name, in jsval value); */
NS_IMETHODIMP
GonkCamera::SetParameter(const nsAString & name, const JS::Value & value, JSContext* cx)
{
  char* k = ToNewCString(name);

  // *** BIG UGLY HACK ***
  if (strcmp(k, "hack-restart-preview") == 0) {
    if (GonkCameraHardware::doCameraHardwareStartPreview(mHwHandle) == OK) {
      mState = HW_STATE_PREVIEW;
    }
    return NS_OK;
  }

  ReentrantMonitorAutoEnter enter(mMonitor);

  if (JSVAL_IS_STRING(value)) {
    // parameter values are all ASCII
    char* v = JS_EncodeString(cx, JSVAL_TO_STRING(value));

    GONKIMPL_LOGI("%s: key '%s' <-- value '%s'\n", __func__, k, v);
    GonkCameraHardware::setCameraHardwareParameter(mHwHandle, k, v);

    JS_free(cx, v);
  }
  nsMemory::Free(k);

  return NS_OK;
}
#endif

void
GonkCamera::ReceiveImage(PRUint8* aData, PRUint32 aLength)
{
  ReentrantMonitorAutoEnter enter(mMonitor);
/*
  Rewrite the callback code

  LOG_REFCNT(this);

  PRUint8* data = new PRUint8[aLength];
  memcpy(data, aData, aLength);
  nsIDOMBlob *blob = new nsDOMMemoryFile((void*)data, (PRUint64)aLength, NS_LITERAL_STRING("image/jpeg"));
  nsCOMPtr<nsIRunnable> resultRunnable = new CameraTakePictureResultTask( blob, mTPOnSuccessCB, mTPOnErrorCB );
  if (NS_FAILED(NS_DispatchToMainThread(resultRunnable))) {
    NS_WARNING("Failed to dispatch to main thread!");
  }

  mState = HW_STATE_PREVIEW_PAUSED;
  LOG_REFCNT(this);
*/
}

void
GonkCamera::AutoFocusComplete(bool success)
{
  ReentrantMonitorAutoEnter enter(mMonitor);
/*
  Rewrite the callback code

  LOG_REFCNT(this);

#if GONKIMPL_TIMING_OVERALL
  struct timespec end;
  float seconds;

  clock_gettime(CLOCK_MONOTONIC, &end);
  timespecSubtract(&mAutoFocusStart, &end);
  seconds = mAutoFocusStart.tv_sec;
  seconds += mAutoFocusStart.tv_nsec / 1000000000;
  printf_stderr("Autofocus took %f seconds\n", seconds);
#endif

  nsCOMPtr<nsIRunnable> resultRunnable = new CameraAutoFocusResultTask( success, mAFOnSuccessCB, mAFOnErrorCB );
  if (NS_FAILED(NS_DispatchToMainThread(resultRunnable))) {
    NS_WARNING("Failed to dispatch to main thread!");
  }

  LOG_REFCNT(this);
*/
}

void
GonkCamera::ReceiveFrame(PRUint8* aData, PRUint32 aLength)
{
#define GONKIMPL_DROP_INITIAL_FRAMES  0

#if GONKIMPL_TIMING_RECEIVEFRAME
  static struct timespec lastFrame;
  struct timespec thisFrame;
  struct timespec thisFrameSlice;

  clock_gettime(CLOCK_MONOTONIC, &thisFrame);
  timespecSubtract(&lastFrame, &thisFrame);
  printf_stderr("ReceiveFrame interval %d s, %d ns\n", lastFrame.tv_sec, lastFrame.tv_nsec);
  lastFrame = thisFrame;
#endif

  if (mState != HW_STATE_PREVIEW) {
    return;
  }
  ReentrantMonitorAutoEnter enter(mMonitor);
  LOG_REFCNT(this);

  if (!mPreviewStarted) {
  #if GONKIMPL_TIMING_OVERALL
    clock_gettime(CLOCK_MONOTONIC, &mStart);
  #endif
    mPreviewStarted = true;
  }

  // GONKIMPL_LOGI("XxXxX Notify\n");
#if GONKIMPL_DROP_INITIAL_FRAMES > 0
  if (mNumFrames >= GONKIMPL_DROP_INITIAL_FRAMES) {
#endif

  // GONKIMPL_LOGI("%s:%d\n", __func__, __LINE__);

#if GONKIMPL_TIMING_RECEIVEFRAME
  clock_gettime(CLOCK_MONOTONIC, &thisFrameSlice);
  timespecSubtract(&thisFrame, &thisFrameSlice);
  printf_stderr("ReceiveFrame post monitor entry %d s, %d ns\n", thisFrame.tv_sec, thisFrame.tv_nsec);
  thisFrame = thisFrameSlice;
#endif

  // create a VideoFrame and push it to the track
  Image::Format format = Image::PLANAR_YCBCR;
  nsRefPtr<Image> image = mImageContainer->CreateImage(&format, 1);
  image->AddRef();
  PlanarYCbCrImage* videoImage = static_cast<PlanarYCbCrImage*> (image.get());

  // GONKIMPL_LOGI("%s:%d\n", __func__, __LINE__);

#if GONKIMPL_TIMING_RECEIVEFRAME
  clock_gettime(CLOCK_MONOTONIC, &thisFrameSlice);
  timespecSubtract(&thisFrame, &thisFrameSlice);
  printf_stderr("ReceiveFrame post CreateImage %d s, %d ns\n", thisFrame.tv_sec, thisFrame.tv_nsec);
  thisFrame = thisFrameSlice;
#endif

  if (!mIs420p) {
  #if 1
    uint8_t* y = aData;
    uint32_t yN = mWidth * mHeight;
    uint32_t uvN = yN / 4;
    uint32_t* src = (uint32_t*)( y + yN );
    uint32_t* d = new uint32_t[ uvN / 2 ];
    uint32_t* u = d;
    uint32_t* v = u + uvN / 4;

    uint32_t u0;
    uint32_t v0;
    uint32_t u1;
    uint32_t v1;

    uint32_t src0;
    uint32_t src1;

    uvN /= 8;

    // GONKIMPL_LOGI("y = %p, yN = %d (mW = %d, mH = %d), uvN = %d, src = %p, u = %p, v = %p\n", y, yN, mWidth, mHeight, uvN, src, u, v);

    while( uvN-- ) {
      src0 = *src++;
      src1 = *src++;

    #if 0
      u0 = ( src0 & 0xFF0000 ) << 8 | ( src0 & 0xFF ) << 16;
      u0 |= ( src1 & 0xFF0000 ) >> 8 | ( src1 & 0xFF );
      v0 = ( src0 & 0xFF000000 ) | ( src0 & 0xFF00 ) << 8;
      v0 |= ( src1 & 0xFF000000 ) >> 16 | ( src1 & 0xFF00 ) >> 8;
    #else
      u0 = ( src0 & 0xFF00UL ) >> 8 | ( src0 & 0xFF000000UL ) >> 16;
      u0 |= ( src1 & 0xFF00UL ) << 8 | ( src1 & 0xFF000000UL );
      v0 = ( src0 & 0xFFUL ) | ( src0 & 0xFF0000UL ) >> 8;
      v0 |= ( src1 & 0xFFUL ) << 16 | ( src1 & 0xFF0000UL ) << 8;
    #endif

      src0 = *src++;
      src1 = *src++;

    #if 0
      u1 = ( src0 & 0xFF0000 ) << 8 | ( src0 & 0xFF ) << 16;
      u1 |= ( src1 & 0xFF0000 ) >> 8 | ( src1 & 0xFF );
      v1 = ( src0 & 0xFF000000 ) | ( src0 & 0xFF00 ) << 8;
      v1 |= ( src1 & 0xFF000000 ) >> 16 | ( src1 & 0xFF00 ) >> 8;
    #else
      u1 = ( src0 & 0xFF00UL ) >> 8 | ( src0 & 0xFF000000UL ) >> 16;
      u1 |= ( src1 & 0xFF00UL ) << 8 | ( src1 & 0xFF000000UL );
      v1 = ( src0 & 0xFFUL ) | ( src0 & 0xFF0000UL ) >> 8;
      v1 |= ( src1 & 0xFFUL ) << 16 | ( src1 & 0xFF0000UL ) << 8;
    #endif

      *u++ = u0;
      *u++ = u1;
      *v++ = v0;
      *v++ = v1;
    }
    // GONKIMPL_LOGI("src = %p, u = %p, v = %p\n", src, u, v);

    memcpy(y + yN, d, yN / 2);
    delete[] d;
  #else
    PRUint8* data = new PRUint8[ aLength ];
    if (!data) {
      GONKIMPL_LOGE("Couldn't allocate de-interlacing buffer\n");
      delete image;
      return;
    }

    // we copy the Y plane, and de-interlace the CrCb
    PRUint32 yFrameSize = mWidth * mHeight;
    PRUint32 uvFrameSize = yFrameSize / 4;
    memcpy(data, aData, yFrameSize);

    PRUint8* uFrame = data + yFrameSize;
    PRUint8* vFrame = data + yFrameSize + uvFrameSize;
    const PRUint8* yFrame = aData + yFrameSize;
    for (PRUint32 i = 0; i < uvFrameSize; i++) {
      uFrame[i] = yFrame[2 * i + 1];
      vFrame[i] = yFrame[2 * i];
    }

    aData = data;
  #endif
  }

#if GONKIMPL_TIMING_RECEIVEFRAME
  clock_gettime(CLOCK_MONOTONIC, &thisFrameSlice);
  timespecSubtract(&thisFrame, &thisFrameSlice);
  printf_stderr("ReceiveFrame post de-interlacing %d s, %d ns\n", thisFrame.tv_sec, thisFrame.tv_nsec);
  thisFrame = thisFrameSlice;
#endif

  const PRUint8 lumaBpp = 8;
  const PRUint8 chromaBpp = 4;
  PlanarYCbCrImage::Data data;
  data.mYChannel = aData;
  data.mYSize = gfxIntSize(mWidth, mHeight);
  data.mYStride = mWidth * lumaBpp / 8.0;
  data.mCbCrStride = mWidth * chromaBpp / 8.0;
  data.mCbChannel = aData + mHeight * data.mYStride;
  data.mCrChannel = data.mCbChannel + mHeight * data.mCbCrStride / 2;
  data.mCbCrSize = gfxIntSize(mWidth / 2, mHeight / 2);
  data.mPicX = 0;
  data.mPicY = 0;
  data.mPicSize = gfxIntSize(mWidth, mHeight);
  data.mStereoMode = mozilla::layers::STEREO_MODE_MONO;
  videoImage->SetData(data); // Copies buffer

#if GONKIMPL_TIMING_RECEIVEFRAME
  clock_gettime(CLOCK_MONOTONIC, &thisFrameSlice);
  timespecSubtract(&thisFrame, &thisFrameSlice);
  printf_stderr("ReceiveFrame post setData() %d s, %d ns\n", thisFrame.tv_sec, thisFrame.tv_nsec);
  thisFrame = thisFrameSlice;
#endif

  mVideoSegment.AppendFrame(videoImage, 1, gfxIntSize(mWidth, mHeight));
  mInput->AppendToTrack(TRACK_VIDEO, &mVideoSegment);
  //PRUint32 time = SecondsToMediaTime(1 + mNumFrames / mFps);
  //mInput->AdvanceKnownTracksTime(time);
#if GONKIMPL_DROP_INITIAL_FRAMES > 0
  }
#endif
  mNumFrames++;
#if GONKIMPL_TIMING_RECEIVEFRAME
  clock_gettime(CLOCK_MONOTONIC, &thisFrameSlice);
  timespecSubtract(&thisFrame, &thisFrameSlice);
  printf_stderr("ReceiveFrame complete %d s, %d ns\n", thisFrame.tv_sec, thisFrame.tv_nsec);
#endif
}

NS_IMPL_ISUPPORTS3(GonkCamera, nsIDOMMediaStream, nsICameraControl, nsIClassInfo)

NS_IMETHODIMP
CameraImpl::GetListOfCameras(JSContext* cx, JS::Value *_retval NS_OUTPARAM)
{
  /*
  camera_module_t* module;

  if (hw_get_module(CAMERA_HARDWARE_MODULE_ID,
            (const hw_module_t **)&module) < 0) {
    GONKIMPL_LOGE("getNumberOfCameras : Could not load camera HAL module");
    return 0;
  }
  return module->get_number_of_cameras();
  */
  
  return NS_OK;
}

NS_IMETHODIMP
CameraImpl::GetCamera(const JS::Value & aOptions, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError, JSContext* cx)
{
  GONKIMPL_LOGI("XxXxX CameraImpl::GetCamera()\n");
  const char* camera = "front";
  bool freeCamera = false;

  if (aOptions.isObject()) {
    // extract values from aOptions
    JSObject *options = JSVAL_TO_OBJECT(aOptions);
    jsval v;

    if (JS_GetProperty(cx, options, "camera", &v)) {
      if (JSVAL_IS_STRING(v)) {
        camera = JS_EncodeString(cx, JSVAL_TO_STRING(v));
        if (camera) {
          freeCamera = true;
        }
      }
    }
  }

  GONKIMPL_LOGA("requested camera '%s'\n", camera);

/*
  TODO: create and dispatch runnable, to get camera, replacing
    the code below.

  nsCOMPtr<nsIDOMMediaStream> stream = new GonkCamera(camera, width, height, fps);
  *_retval = stream.get();
  NS_ADDREF(stream);
*/

  if (freeCamera) {
    JS_free(cx, const_cast<char*>(camera));
  }  
  return NS_OK;
}

void GonkCameraReceiveImage(GonkCamera* gc, PRUint8* aData, PRUint32 aLength)
{
  gc->ReceiveImage(aData, aLength);
}

void GonkCameraAutoFocusComplete(GonkCamera* gc, bool success)
{
  gc->AutoFocusComplete(success);
}

void GonkCameraReceiveFrame(GonkCamera* gc, PRUint8* aData, PRUint32 aLength)
{
  gc->ReceiveFrame(aData, aLength);
}

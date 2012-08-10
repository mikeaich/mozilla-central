/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VideoUtils.h"
#include "DOMCameraPreview.h"

#define DOM_CAMERA_DEBUG_REFS 1
#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;

/**
 * 'PreviewControl' is a helper class that dispatches preview control
 * events from the main thread.
 *
 * NS_NewRunnableMethod() can't be used because it AddRef()s the method's
 * object, which can't be done off the main thread for cycle collection
 * participants.
 *
 * Before using this class, 'aDOMPreview' must be appropriately AddRef()ed.
 */
class PreviewControl : public nsRunnable
{
public:
  enum {
    START,
    STOP,
    STARTED,
    STOPPED
  };
  PreviewControl(DOMCameraPreview* aDOMPreview, PRUint32 aControl)
    : mDOMPreview(aDOMPreview)
    , mControl(aControl)
  { }

  NS_IMETHOD Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "PreviewControl not run on main thread");

    switch (mControl) {
      case START:
        mDOMPreview->Start();
        break;

      case STOP:
        mDOMPreview->Stop();
        break;

      case STARTED:
        mDOMPreview->SetStateStarted();
        break;

      case STOPPED:
        mDOMPreview->SetStateStopped();
        break;

      default:
        DOM_CAMERA_LOGE("PreviewControl: invalid control %d\n", mControl);
        break;
    }

    return NS_OK;
  }

protected:
  /**
   * This must be a raw pointer because this class is not created on the
   * main thread, and DOMCameraPreview is not threadsafe.  Prior to
   * issuing a preview control event, the caller must ensure that
   * mDOMPreview will not disappear.
   */
  DOMCameraPreview* mDOMPreview;
  PRUint32 mControl;
};

class DOMCameraPreviewListener : public MediaStreamListener
{
public:
  DOMCameraPreviewListener(DOMCameraPreview* aDOMPreview) :
    mDOMPreview(aDOMPreview)
  {
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  ~DOMCameraPreviewListener()
  {
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  void NotifyConsumptionChanged(MediaStreamGraph* aGraph, Consumption aConsuming)
  {
    const char* state;

    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);

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

    DOM_CAMERA_LOGA("camera viewfinder is %s\n", state);
    nsCOMPtr<nsIRunnable> previewControl;

    switch (aConsuming) {
      case NOT_CONSUMED:
        previewControl = new PreviewControl(mDOMPreview, PreviewControl::STOP);
        break;

      case CONSUMED:
        previewControl = new PreviewControl(mDOMPreview, PreviewControl::START);
        break;

      default:
        return;
    }

    nsresult rv = NS_DispatchToMainThread(previewControl);
    if (NS_FAILED(rv)) {
      DOM_CAMERA_LOGE("Failed to dispatch preview control (%d)!\n", rv);
    }
  }

protected:
  nsCOMPtr<DOMCameraPreview> mDOMPreview;
};

DOMCameraPreview::DOMCameraPreview(ICameraControl* aCameraControl, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aFrameRate)
  : nsDOMMediaStream()
  , mState(STOPPED)
  , mWidth(aWidth)
  , mHeight(aHeight)
  , mFramesPerSecond(aFrameRate)
  , mFrameCount(0)
  , mCameraControl(aCameraControl)
{
  DOM_CAMERA_LOGI("%s:%d : mWidth=%d, mHeight=%d, mFramesPerSecond=%d : this=%p\n", __func__, __LINE__, mWidth, mHeight, mFramesPerSecond, this);

  mImageContainer = LayerManager::CreateImageContainer();
  MediaStreamGraph* gm = MediaStreamGraph::GetInstance();
  mStream = gm->CreateInputStream(this);
  mInput = GetStream()->AsSourceStream();
  mInput->AddListener(new DOMCameraPreviewListener(this));

  mInput->AddTrack(TRACK_VIDEO, mFramesPerSecond, 0, new VideoSegment());
  mInput->AdvanceKnownTracksTime(MEDIA_TIME_MAX);
}

DOMCameraPreview::~DOMCameraPreview()
{
  DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
}

bool
DOMCameraPreview::HaveEnoughBuffered()
{
  return mInput->HaveEnoughBuffered(TRACK_VIDEO);
}

void
DOMCameraPreview::ReceiveFrame(PRUint8* aFrame)
{
  if (mState != STARTED) {
    return;
  }

  Image::Format format = Image::PLANAR_YCBCR;
  nsRefPtr<Image> image = mImageContainer->CreateImage(&format, 1);
  image->AddRef();
  PlanarYCbCrImage* videoImage = static_cast<PlanarYCbCrImage*>(image.get());

  /**
   * If you change either lumaBpp or chromaBpp, make sure the
   * assertions below still hold.
   */
  const PRUint8 lumaBpp = 8;
  const PRUint8 chromaBpp = 4;
  PlanarYCbCrImage::Data data;
  data.mYChannel = aFrame;
  data.mYSize = gfxIntSize(mWidth, mHeight);

  data.mYStride = mWidth * lumaBpp;
  NS_ASSERTION((data.mYStride & 0x7) == 0, "Invalid image dimensions!");
  data.mYStride /= 8;

  data.mCbCrStride = mWidth * chromaBpp;
  NS_ASSERTION((data.mCbCrStride & 0x7) == 0, "Invalid image dimensions!");
  data.mCbCrStride /= 8;

  data.mCbChannel = aFrame + mHeight * data.mYStride;
  data.mCrChannel = data.mCbChannel + mHeight * data.mCbCrStride / 2;
  data.mCbCrSize = gfxIntSize(mWidth / 2, mHeight / 2);
  data.mPicX = 0;
  data.mPicY = 0;
  data.mPicSize = gfxIntSize(mWidth, mHeight);
  data.mStereoMode = mozilla::layers::STEREO_MODE_MONO;
  videoImage->SetData(data); // copies buffer

  mVideoSegment.AppendFrame(videoImage, 1, gfxIntSize(mWidth, mHeight));
  mInput->AppendToTrack(TRACK_VIDEO, &mVideoSegment);
}

void
DOMCameraPreview::Start()
{
  NS_ASSERTION(NS_IsMainThread(), "Start() not called from main thread");
  if (mState != STOPPED) {
    return;
  }

  DOM_CAMERA_LOGI("Starting preview stream\n");
  
  /**
   * Add a reference to ourselves to make sure we stay alive while
   * the preview is running, as the CameraControlImpl object holds a
   * weak reference to us.
   *
   * This reference is removed in SetStateStopped().
   */
  NS_ADDREF_THIS();
  mState = STARTING;
  mCameraControl->StartPreview(this);
}

void
DOMCameraPreview::SetStateStarted()
{
  mState = STARTED;
  DOM_CAMERA_LOGI("Preview stream started\n");
}

void
DOMCameraPreview::Started()
{
  if (mState != STARTING) {
    return;
  }

  DOM_CAMERA_LOGI("Dispatching preview stream started\n");
  nsCOMPtr<nsIRunnable> started = new PreviewControl(this, PreviewControl::STARTED);
  nsresult rv = NS_DispatchToMainThread(started);
  if (NS_FAILED(rv)) {
    DOM_CAMERA_LOGE("failed to set statrted state (%d), POTENTIAL MEMORY LEAK!\n", rv);
  }
}

void
DOMCameraPreview::Stop()
{
  NS_ASSERTION(NS_IsMainThread(), "Stop() not called from main thread");
  if (mState != STARTED) {
    return;
  }

  DOM_CAMERA_LOGI("Stopping preview stream\n");
  mState = STOPPING;
  mCameraControl->StopPreview();
  mInput->EndTrack(TRACK_VIDEO);
  mInput->Finish();
}

void
DOMCameraPreview::SetStateStopped()
{
  mState = STOPPED;
  DOM_CAMERA_LOGI("Preview stream stopped\n");
  
  /**
   * Only remove the reference added in Start() once the preview
   * has stopped completely.
   */
  NS_RELEASE_THIS();
}

void
DOMCameraPreview::Stopped(bool aForced)
{
  if (mState != STOPPING && !aForced) {
    return;
  }

  DOM_CAMERA_LOGI("Dispatching preview stream stopped\n");
  nsCOMPtr<nsIRunnable> stopped = new PreviewControl(this, PreviewControl::STOPPED);
  nsresult rv = NS_DispatchToMainThread(stopped);
  if (NS_FAILED(rv)) {
    DOM_CAMERA_LOGE("failed to decrement reference count (%d), MEMORY LEAK!\n", rv);
  }
}

void
DOMCameraPreview::Error()
{
  DOM_CAMERA_LOGE("Error occurred changing preview state!\n");
  Stopped(true);
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VideoUtils.h"
#include "DOMCameraPreview.h"

#define DOM_CAMERA_DEBUG_REFS 1
#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;

class DOMCameraPreviewListener : public MediaStreamListener
{
public:
  DOMCameraPreviewListener(DOMCameraPreview* aPreview) :
    mPreview(aPreview)
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

    switch (aConsuming) {
      case NOT_CONSUMED:
        mPreview->Stop();
        break;

      case CONSUMED:
        mPreview->Start();
        break;
    }
  }

protected:
  nsCOMPtr<DOMCameraPreview> mPreview;
};

DOMCameraPreview::DOMCameraPreview(CameraControl* aCameraControl, PRUint32 aDesiredWidth, PRUint32 aDesiredHeight, PRUint32 aDesiredFrameRate)
  : nsDOMMediaStream()
  , mState(UNINITED)
  , mWidth(aDesiredWidth)
  , mHeight(aDesiredHeight)
  , mFramesPerSecond(aDesiredFrameRate)
  , mFrameCount(0)
  , mCameraControl(aCameraControl)
{
  DOM_CAMERA_LOGI("%s:%d : mWidth=%d, mHeight=%d, mFramesPerSecond=%d : this=%p\n", __func__, __LINE__, mWidth, mHeight, mFramesPerSecond, this);

  mImageContainer = LayerManager::CreateImageContainer();
  MediaStreamGraph* gm = MediaStreamGraph::GetInstance();
  mStream = gm->CreateInputStream(this);
  mInput = GetStream()->AsSourceStream();
  mInput->AddListener(new DOMCameraPreviewListener(this));
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
DOMCameraPreview::ReceiveFrame(PlanarYCbCrImage* aFrame)
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

  data.mCbChannel = aData + mHeight * data.mYStride;
  data.mCrChannel = data.mCbChannel + mHeight * data.mCbCrStride / 2;
  data.mCbCrSize = gfxIntSize(mWidth / 2, mHeight / 2);
  data.mPicX = 0;
  data.mPicY = 0;
  data.mPicSize = gfxIntSize(mWidth, mHeight);
  data.mStereoMode = mozilla::layers::STEREO_MODE_MONO;
  videoImage->SetData(data); // copies buffer

  mVideoSegment.AppendFrame(aFrame, 1, gfxIntSize(mWidth, mHeight));
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
DOMCameraPreview::Started(PRUint32 aActualWidth, PRUint32 aActualHeight, PRUint32 aActualFramesPerSecond)
{
  if (mState != STARTING) {
    return;
  }

  mWidth = aActualWidth;
  mHeight = aActualHeight;
  mFramesPerSecond = aActualFramesPerSecond;
  mInput->AddTrack(TRACK_VIDEO, mFramesPerSecond, 0, new VideoSegment());
  mInput->AdvanceKnownTracksTime(MEDIA_TIME_MAX);

  DOM_CAMERA_LOGI("Dispatching preview stream started\n");
  nsCOMPtr<nsIRunnable> started = NS_NewRunnableMethod(this, &DOMCameraPreview::SetStateStarted);
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
  NS_RELEASE_THIS();
}

void
DOMCameraPreview::Stopped()
{
  if (mState != STOPPING) {
    return;
  }

  DOM_CAMERA_LOGI("Dispatching preview stream stopped\n");
  nsCOMPtr<nsIRunnable> stopped = NS_NewRunnableMethod(this, &DOMCameraPreview::SetStateStopped);
  nsresult rv = NS_DispatchToMainThread(stopped);
  if (NS_FAILED(rv)) {
    DOM_CAMERA_LOGE("failed to decrement reference count (%d), MEMORY LEAK!\n", rv);
  }
}

void
DOMCameraPreview::SetStateError()
{
  mState = UNINITED;
  DOM_CAMERA_LOGI("Preview stream encountered an error\n");
  NS_RELEASE_THIS();
}

void
DOMCameraPreview::Error()
{
  DOM_CAMERA_LOGE("Error occurred changing preview state!\n");
  nsCOMPtr<nsIRunnable> stopped = NS_NewRunnableMethod(this, &DOMCameraPreview::SetStateError);
  nsresult rv = NS_DispatchToMainThread(stopped);
  if (NS_FAILED(rv)) {
    DOM_CAMERA_LOGE("failed to decrement reference count (%d), MEMORY LEAK!\n", rv);
  }
}

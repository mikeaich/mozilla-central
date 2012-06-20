/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VideoUtils.h"
#include "GonkCameraHwMgr.h"
#include "CameraPreview.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"


static const TrackID TRACK_AUDIO = 1;
static const TrackID TRACK_VIDEO = 2;

NS_IMPL_ISUPPORTS1(CameraPreview, CameraPreview)

CameraPreview::CameraPreview(PRUint32 aHwHandle, PRUint32 aWidth, PRUint32 aHeight)
  : nsDOMMediaStream()
  , mHwHandle(aHwHandle)
  , mWidth(aWidth)
  , mHeight(aHeight)
  , mFramesPerSecond(0)
  , mIs420p(false)
  , mFrameCount(0)
{
  DOM_CAMERA_LOGI("%s:%d : mWidth = %d, mHeight = %d\n", __func__, __LINE__, mWidth, mHeight);

  mImageContainer = LayerManager::CreateImageContainer();
  MediaStreamGraph *gm = MediaStreamGraph::GetInstance();
  mStream = gm->CreateInputStream(this);
  mInput = GetStream()->AsSourceStream();
  mInput->AddListener(this);
}

CameraPreview::~CameraPreview()
{
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
  GonkCameraHardware::doCameraHardwareStopPreview(mHwHandle);
  
  /* We _must_ remember to call RemoveListener on this before destroying this,
     else the media framework will trigger a double-free. */
  mInput->RemoveListener(this);
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
}

void
CameraPreview::NotifyConsumptionChanged(MediaStreamGraph* aGraph, Consumption aConsuming)
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
  
  DOM_CAMERA_LOGA("camera viewfinder is %s\n", state);
  
  switch (aConsuming) {
    case NOT_CONSUMED:
      GonkCameraHardware::doCameraHardwareStopPreview(mHwHandle);
      break;
    
    case CONSUMED:
      GonkCameraHardware::setCameraHardwarePreviewSize(mHwHandle, mWidth, mHeight);
      GonkCameraHardware::getCameraHardwarePreviewSize(mHwHandle, &mWidth, &mHeight);
      if (GonkCameraHardware::doCameraHardwareStartPreview(mHwHandle) == OK) {
        // mState = HW_STATE_PREVIEW;
        mFramesPerSecond = GonkCameraHardware::getCameraHardwareFps(mHwHandle);
        DOM_CAMERA_LOGI("preview stream is (actually!) %d x %d (w x h), %d frames per second\n", mWidth, mHeight, mFramesPerSecond);
        mInput->AddTrack(TRACK_VIDEO, mFramesPerSecond, 0, new VideoSegment());
        mInput->AdvanceKnownTracksTime(MEDIA_TIME_MAX);
      } else {
        DOM_CAMERA_LOGE("%s: failed to start preview\n", __func__);
      }
      break;
  }
}

void
CameraPreview::ReceiveFrame(PRUint8 *aData, PRUint32 aLength)
{
  Image::Format format = Image::PLANAR_YCBCR;
  nsRefPtr<Image> image = mImageContainer->CreateImage(&format, 1);
  image->AddRef();
  PlanarYCbCrImage *videoImage = static_cast<PlanarYCbCrImage*>(image.get());

  if (!mIs420p) {
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

    while( uvN-- ) {
      src0 = *src++;
      src1 = *src++;

      u0 = ( src0 & 0xFF00UL ) >> 8 | ( src0 & 0xFF000000UL ) >> 16;
      u0 |= ( src1 & 0xFF00UL ) << 8 | ( src1 & 0xFF000000UL );
      v0 = ( src0 & 0xFFUL ) | ( src0 & 0xFF0000UL ) >> 8;
      v0 |= ( src1 & 0xFFUL ) << 16 | ( src1 & 0xFF0000UL ) << 8;

      src0 = *src++;
      src1 = *src++;

      u1 = ( src0 & 0xFF00UL ) >> 8 | ( src0 & 0xFF000000UL ) >> 16;
      u1 |= ( src1 & 0xFF00UL ) << 8 | ( src1 & 0xFF000000UL );
      v1 = ( src0 & 0xFFUL ) | ( src0 & 0xFF0000UL ) >> 8;
      v1 |= ( src1 & 0xFFUL ) << 16 | ( src1 & 0xFF0000UL ) << 8;

      *u++ = u0;
      *u++ = u1;
      *v++ = v0;
      *v++ = v1;
    }

    memcpy(y + yN, d, yN / 2);
    delete[] d;
  }

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

  mVideoSegment.AppendFrame(videoImage, 1, gfxIntSize(mWidth, mHeight));
  mInput->AppendToTrack(TRACK_VIDEO, &mVideoSegment);

  mFrameCount += 1;

  if ((mFrameCount % 10) == 0) {
    DOM_CAMERA_LOGI("%s:%d : mFrameCount = %d\n", __func__, __LINE__, mFrameCount);
  }
}

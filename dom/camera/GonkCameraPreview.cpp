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

#include "VideoUtils.h"
#include "GonkCameraHwMgr.h"
#include "GonkCameraPreview.h"

#define DOM_CAMERA_LOG_LEVEL  2
#include "CameraCommon.h"


USING_CAMERA_NAMESPACE

#define YUV_CONVERT_INPLACE   0

void
GonkCameraPreview::ReceiveFrame(PRUint8 *aData, PRUint32 aLength)
{
  DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);

  if (mInput->HaveEnoughBuffered(TRACK_VIDEO)) {
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
        /* de-interlace the u and v planes */
#if YUV_CONVERT_INPLACE
        uint8_t* y = aData;
        uint32_t yN = mWidth * mHeight;
        uint32_t uvN = yN / 4;
        uint32_t* src = (uint32_t*)( y + yN );
        uint32_t* d = new uint32_t[ uvN / 2 ];
        uint32_t* u = d;
        uint32_t* v = u + uvN / 4;

        /* we're handling pairs of 32-bit words, so divide by 8 */
        uvN /= 8;

        while( uvN-- ) {
          uint32_t src0 = *src++;
          uint32_t src1 = *src++;

          uint32_t u0;
          uint32_t v0;
          uint32_t u1;
          uint32_t v1;

#define DEINTERLACE( u, v, s0, s1 )                             \
  u = ( (s0) & 0xFF00UL ) >> 8 | ( (s0) & 0xFF000000UL ) >> 16; \
  u |= ( (s1) & 0xFF00UL ) << 8 | ( (s1) & 0xFF000000UL );      \
  v = ( (s0) & 0xFFUL ) | ( (s0) & 0xFF0000UL ) >> 8;           \
  v |= ( (s1) & 0xFFUL ) << 16 | ( (s1) & 0xFF0000UL ) << 8;

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
#else
        PRUint8* data = new PRUint8[ aLength ];
        if (!data) {
          DOM_CAMERA_LOGE("Couldn't allocate de-interlacing buffer\n");
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
      break;

    case GonkCameraHardware::PREVIEW_FORMAT_YUV420P:
      /* no transformating required */
      break;

    default:
      /* in a format we don't handle, get out of here */
      return;
  }

  Image::Format format = Image::PLANAR_YCBCR;
  nsRefPtr<Image> image = mImageContainer->CreateImage(&format, 1);
  image->AddRef();
  PlanarYCbCrImage *videoImage = static_cast<PlanarYCbCrImage*>(image.get());

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

void
GonkCameraPreview::Start()
{
  DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);

  GonkCameraHardware::SetPreviewSize(mHwHandle, mWidth, mHeight);
  GonkCameraHardware::GetPreviewSize(mHwHandle, &mWidth, &mHeight);
  SetFrameRate(GonkCameraHardware::GetFps(mHwHandle));

  if (GonkCameraHardware::StartPreview(mHwHandle) == OK) {
    mFormat = GonkCameraHardware::GetPreviewFormat(mHwHandle);
    DOM_CAMERA_LOGI("preview stream is (actually!) %d x %d (w x h), %d frames per second, format %d\n", mWidth, mHeight, mFramesPerSecond, mFormat);
  } else {
    DOM_CAMERA_LOGE("%s: failed to start preview\n", __func__);
  }
}

void
GonkCameraPreview::Stop()
{
  DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);

  GonkCameraHardware::StopPreview(mHwHandle);
  mInput->EndTrack(TRACK_VIDEO);
  mInput->Finish();
}

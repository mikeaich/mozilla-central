/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_CAMERAPREVIEW_H
#define DOM_CAMERA_CAMERAPREVIEW_H

#include "MediaStreamGraph.h"
#include "StreamBuffer.h"
#include "nsDOMMediaStream.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;
using namespace mozilla::layers;

BEGIN_CAMERA_NAMESPACE

class CameraPreview : public nsDOMMediaStream
                    , public MediaStreamListener
{
public:
  NS_DECL_ISUPPORTS

  CameraPreview(PRUint32 aWidth, PRUint32 aHeight);
  virtual ~CameraPreview();

  void SetFrameRate(PRUint32 aFramesPerSecond);

  NS_IMETHODIMP
  GetCurrentTime(double* aCurrentTime) {
    return nsDOMMediaStream::GetCurrentTime(aCurrentTime);
  }

  virtual void Start() = 0;
  virtual void Stop() = 0;

protected:
  PRUint32 mWidth;
  PRUint32 mHeight;
  PRUint32 mFramesPerSecond;
  SourceMediaStream *mInput;
  nsRefPtr<mozilla::layers::ImageContainer> mImageContainer;
  VideoSegment mVideoSegment;
  PRUint32 mFrameCount;

  enum { TRACK_VIDEO = 1 };

private:
  CameraPreview(const CameraPreview&);
  CameraPreview& operator=(const CameraPreview&);
};

END_CAMERA_NAMESPACE

#endif // DOM_CAMERA_CAMERAPREVIEW_H

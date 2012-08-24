/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_DOMCAMERAPREVIEW_H
#define DOM_CAMERA_DOMCAMERAPREVIEW_H

#include "nsCycleCollectionParticipant.h"
#include "MediaStreamGraph.h"
#include "StreamBuffer.h"
#include "ICameraControl.h"
#include "GonkIOSurfaceImage.h"
#include "nsDOMMediaStream.h"

#define DOM_CAMERA_DEBUG_REFS 1
#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;
using namespace mozilla::layers;

namespace mozilla {
  
class DOMCameraPreview : public nsDOMMediaStream
{
protected:
  enum { TRACK_VIDEO = 1 };

public:
  DOMCameraPreview(ICameraControl* aCameraControl, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aFramesPerSecond = 30);
  void ReceiveFrame(layers::GraphicBufferLocked* aBuffer, ImageFormat aFormat);
  bool HaveEnoughBuffered();

  NS_IMETHODIMP
  GetCurrentTime(double* aCurrentTime) {
    return nsDOMMediaStream::GetCurrentTime(aCurrentTime);
  }

  void Start();   // called by the MediaStreamListener to start preview
  void Started(); // called by the CameraControl when preview is started
  void Stop();    // called by the MediaStreamListener to stop preview
  void Stopped(bool aForced = false);
                  // called by the CameraControl when preview is stopped
  void Error();   // something went wrong, NS_RELEASE needed

  void SetStateStarted();
  void SetStateStopped();

protected:
  virtual ~DOMCameraPreview();

  enum {
    STOPPED,
    STARTING,
    STARTED,
    STOPPING
  };
  PRUint32 mState;

  PRUint32 mWidth;
  PRUint32 mHeight;
  PRUint32 mFramesPerSecond;
  SourceMediaStream* mInput;
  nsRefPtr<ImageContainer> mImageContainer;
  VideoSegment mVideoSegment;
  PRUint32 mFrameCount;
  nsRefPtr<ICameraControl> mCameraControl;

  // Raw pointer; AddListener() keeps the reference for us
  MediaStreamListener* mListener;

private:
  DOMCameraPreview(const DOMCameraPreview&) MOZ_DELETE;
  DOMCameraPreview& operator=(const DOMCameraPreview&) MOZ_DELETE;
};

} // namespace mozilla

#endif // DOM_CAMERA_DOMCAMERAPREVIEW_H

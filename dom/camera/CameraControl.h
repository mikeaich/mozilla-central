/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_NSCAMERACONTROL_H
#define DOM_CAMERA_NSCAMERACONTROL_H


#include "prtypes.h"
#include "nsThread.h"
#include "CameraPreview.h"
#include "nsIDOMCameraManager.h"


class nsCameraControl : public nsICameraControl
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICAMERACONTROL

  const char* GetParameter(const char* key);
  void SetParameter(const char* key, const char* value);

  void ReceiveImage(PRUint8* aData, PRUint32 aLength);
  void AutoFocusComplete(bool success);
  void ReceiveFrame(PRUint8* aData, PRUint32 aLength);

  PRUint32 GetHwHandle()
  {
    return mHwHandle;
  }
  void SetPreview(CameraPreview *aPreview)
  {
    mPreview = aPreview;
  }

  nsCameraControl(PRUint32 aCameraId, nsIThread *aCameraThread);
  ~nsCameraControl();

private:
  nsCameraControl(const nsCameraControl&);

protected:
  /* additional members */
  PRUint32                        mCameraId;
  nsCOMPtr<nsIThread>             mCameraThread;
  nsRefPtr<nsICameraCapabilities> mCapabilities;
  PRUint32                        mHwHandle;
  PRUint32                        mPreviewWidth;
  PRUint32                        mPreviewHeight;
  nsCOMPtr<CameraPreview>         mPreview;
};


class GetPreviewStreamResult : public nsRunnable
{
public:
  GetPreviewStreamResult(nsIDOMMediaStream *aStream, nsICameraPreviewStreamCallback *onSuccess)
    : mStream(aStream)
    , mOnSuccessCb(onSuccess)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    
    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mStream);
    }
    return NS_OK;
  }

protected:
  nsCOMPtr<nsIDOMMediaStream> mStream;
  nsCOMPtr<nsICameraPreviewStreamCallback> mOnSuccessCb;
};

class DoGetPreviewStream : public nsRunnable
{
public:
  DoGetPreviewStream(nsCameraControl *aCameraControl, PRUint32 aWidth, PRUint32 aHeight, nsICameraPreviewStreamCallback *onSuccess, nsICameraErrorCallback *onError)
    : mWidth(aWidth)
    , mHeight(aHeight)
    , mCameraControl(aCameraControl)
    , mOnSuccessCb(onSuccess)
  { }

  NS_IMETHOD Run()
  {
    nsCOMPtr<CameraPreview> preview = new CameraPreview(mCameraControl->GetHwHandle(), mWidth, mHeight);

    mCameraControl->SetPreview(preview);
    if (NS_FAILED(NS_DispatchToMainThread(new GetPreviewStreamResult(preview.get(), mOnSuccessCb)))) {
      NS_WARNING("Failed to dispatch getPreviewStream() onSuccess callback to main thread!");
    }
    return NS_OK;
  }

protected:
  PRUint32 mWidth;
  PRUint32 mHeight;
  nsCOMPtr<nsCameraControl> mCameraControl;
  nsCOMPtr<nsICameraPreviewStreamCallback> mOnSuccessCb;
};


#endif // DOM_CAMERA_NSCAMERACONTROL_H

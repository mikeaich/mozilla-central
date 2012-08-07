/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_NSCAMERACONTROL_H
#define DOM_CAMERA_NSCAMERACONTROL_H

#include "prtypes.h"
#include "nsCOMPtr.h"
#include "nsDOMFile.h"
#include "DictionaryHelpers.h"
#include "CameraCore.h"
#include "CameraPreview.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMCameraManager.h"

#define DOM_CAMERA_LOG_LEVEL 3
#include "CameraCommon.h"

namespace mozilla {

using namespace dom;

// Main camera control.
class nsCameraControl : public nsICameraControl
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsCameraControl)
  NS_DECL_NSICAMERACONTROL

  nsCameraControl(PRUint32 aCameraId, nsIThread* aCameraThread)
    : mCameraId(aCameraId)
    , mCameraThread(aCameraThread)
    , mCapabilities(nullptr)
    , mPreview(nullptr)
  {
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
    mCameraCore = new CameraCore(mCameraId, mCameraThread);
  }

protected:
  virtual ~nsCameraControl() { }

  nsresult SetHelper(PRUint32 aKey, const nsAString& aValue);
  nsresult GetHelper(PRUint32 aKey, nsAString& aValue);
  nsresult SetHelper(PRUint32 aKey, double aValue);
  nsresult GetHelper(PRUint32 aKey, double* aValue);
  nsresult SetHelper(JSContext* aCx, PRUint32 aKey, const JS::Value& aValue, PRUint32 aLimit);
  nsresult GetHelper(JSContext* aCx, PRUint32 aKey, JS::Value* aValue);

private:
  nsCameraControl(const nsCameraControl&) MOZ_DELETE;
  nsCameraControl& operator=(const nsCameraControl&) MOZ_DELETE;

protected:
  /* additional members */
  nsRefPtr<CameraCore>            mCameraCore;
  PRUint32                        mCameraId;
  nsCOMPtr<nsIThread>             mCameraThread;
  nsCOMPtr<nsICameraCapabilities> mCapabilities;
  PRUint32                        mPreviewWidth;
  PRUint32                        mPreviewHeight;
  nsCOMPtr<CameraPreview>         mPreview;
};

} // namespace mozilla

#endif // DOM_CAMERA_NSCAMERACONTROL_H

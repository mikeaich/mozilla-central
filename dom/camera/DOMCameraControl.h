/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_nsDOMCameraControl_H
#define DOM_CAMERA_nsDOMCameraControl_H

#include "prtypes.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "DictionaryHelpers.h"
#include "CameraControl.h"
#include "DOMCameraPreview.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMCameraManager.h"

#define DOM_CAMERA_LOG_LEVEL 3
#include "CameraCommon.h"

namespace mozilla {

using namespace dom;

// Main camera control.
class nsDOMCameraControl : public nsICameraControl
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsDOMCameraControl)
  NS_DECL_NSICAMERACONTROL

  nsDOMCameraControl(PRUint32 aCameraId, nsIThread* aCameraThread, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError);

protected:
  virtual ~nsDOMCameraControl() { }

private:
  nsDOMCameraControl(const nsDOMCameraControl&) MOZ_DELETE;
  nsDOMCameraControl& operator=(const nsDOMCameraControl&) MOZ_DELETE;

protected:
  /* additional members */
  nsRefPtr<CameraControl>         mCameraControl; // non-DOM implementation
  PRUint32                        mCameraId;
  nsCOMPtr<nsIThread>             mCameraThread;
  nsCOMPtr<nsICameraCapabilities> mDOMCapabilities;
  PRUint32                        mPreviewWidth;
  PRUint32                        mPreviewHeight;
  nsCOMPtr<DOMCameraPreview>      mDOMPreview;
};

} // namespace mozilla

#endif // DOM_CAMERA_nsDOMCameraControl_H

/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=40: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_DOMCAMERAMANAGER_H
#define DOM_CAMERA_DOMCAMERAMANAGER_H


#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIThread.h"
#include "nsThreadUtils.h"
#include "nsIDOMCameraManager.h"


class nsDOMCameraManager : public nsIDOMCameraManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMCAMERAMANAGER

  static NS_IMETHODIMP Create(PRUint64 aWindowId, nsDOMCameraManager * *aMozCameras);
  static nsRefPtr<nsDOMCameraManager> sCameraManager;

  void OnNavigation(PRUint64 aWindowId);

private:
  nsDOMCameraManager();
  nsDOMCameraManager(PRUint64 aWindowId);
  nsDOMCameraManager(const nsDOMCameraManager&);
  nsDOMCameraManager& operator=(const nsDOMCameraManager&);
  ~nsDOMCameraManager();

protected:
  PRUint64 mWindowId;
  nsCOMPtr<nsIThread> mCameraThread;
};


class DoGetCamera : public nsRunnable
{
public:
  DoGetCamera(PRUint32 aCameraId, nsICameraGetCameraCallback *onSuccess, nsICameraErrorCallback *onError, nsIThread *aCameraThread)
    : mCameraId(aCameraId)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
    , mCameraThread(aCameraThread)
  { }

  NS_IMETHOD Run();

protected:
  PRUint32 mCameraId;
  nsCOMPtr<nsICameraGetCameraCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
  nsCOMPtr<nsIThread> mCameraThread;
};

class GetCameraResult : public nsRunnable
{
public:
  GetCameraResult(nsICameraControl *aCameraControl, nsICameraGetCameraCallback *onSuccess)
    : mCameraControl(aCameraControl)
    , mOnSuccessCb(onSuccess)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    /* TO DO: window management stuff */
    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mCameraControl);
    }
    return NS_OK;
  }

protected:
  nsCOMPtr<nsICameraControl> mCameraControl;
  nsCOMPtr<nsICameraGetCameraCallback> mOnSuccessCb;
};


#endif // DOM_CAMERA_DOMCAMERAMANAGER_H

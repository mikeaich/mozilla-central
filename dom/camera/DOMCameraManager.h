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

  static already_AddRefed<nsDOMCameraManager> Create(PRUint64 aWindowId);

  void OnNavigation(PRUint64 aWindowId);

private:
  nsDOMCameraManager();
  nsDOMCameraManager(PRUint64 aWindowId);
  nsDOMCameraManager(const nsDOMCameraManager&) MOZ_DELETE;
  nsDOMCameraManager& operator=(const nsDOMCameraManager&) MOZ_DELETE;
  ~nsDOMCameraManager();

protected:
  PRUint64 mWindowId;
  nsCOMPtr<nsIThread> mCameraThread;
};

#endif // DOM_CAMERA_DOMCAMERAMANAGER_H

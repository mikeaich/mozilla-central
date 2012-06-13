/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DOMCameraManager.h"
#include "nsDOMClassInfo.h"


NS_IMPL_ISUPPORTS1(nsDOMCameraManager, nsIDOMCameraManager)

DOMCI_DATA(CameraManager, nsIDOMCameraManager)

/*
  nsDOMCameraManager::GetCamera() and nsDOMCameraManager::GetListOfCameras
  are implementation-specific, and can be found in (e.g.)
  GonkCameraControl.cpp and FallbackCameraControl.cpp.
*/

nsDOMCameraManager::nsDOMCameraManager(PRUint64 aWindowId)
  : mWindowId(aWindowId)
{
  /* member initializers and constructor code */
}

nsDOMCameraManager::~nsDOMCameraManager()
{
  /* destructor code */
}

void
nsDOMCameraManager::OnNavigation(PRUint64 aWindowId)
{
}

/* static creator */
NS_IMETHODIMP
nsDOMCameraManager::Create(PRUint64 aWindowId, nsDOMCameraManager * *aMozCameras)
{
  /* TODO: check for permissions here to access cameras */

  nsRefPtr<nsDOMCameraManager> cameraManager = new nsDOMCameraManager(aWindowId);
  cameraManager.forget(aMozCameras);
  return NS_OK;
}

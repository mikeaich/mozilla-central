/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DOMCameraManager.h"
#include "DOMNavigatorCamera.h"


NS_IMPL_ISUPPORTS1(nsDOMNavigatorCamera, nsIDOMNavigatorCamera)

nsDOMNavigatorCamera::nsDOMNavigatorCamera()
{
  /* member initializers and constructor code */
}

nsDOMNavigatorCamera::~nsDOMNavigatorCamera()
{
  /* destructor code */
}

/* readonly attribute nsIDOMCameraManager mozCameras; */
NS_IMETHODIMP nsDOMNavigatorCamera::GetMozCameras(PRUint64 aWindowId, nsIDOMCameraManager * *aMozCameras)
{
  /* TODO: check for permissions here to access cameras */

  nsRefPtr<nsIDOMCameraManager> cameraManager = nsDOMCameraManager::Create(aWindowId);
  cameraManager.forget(aMozCameras)
  return NS_OK;
}

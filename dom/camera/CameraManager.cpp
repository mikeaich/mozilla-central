/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CameraManager.h"

USING_CAMERA_NAMESPACE










nsresult
NS_NewCameraManager(nsPIDOMWindow* aWindowId, nsIDOMCameraManager** aCameraManager)
{
  nsRefPtr<CameraManager> cameraManager = CameraManager::Create(aWindowId);

  cameraManager.forget(aCameraManager);
  return NS_OK;
}

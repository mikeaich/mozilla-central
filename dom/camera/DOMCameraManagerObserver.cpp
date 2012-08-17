/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/Services.h"
#include "nsObserverService.h"
#include "nsISupportsPrimitives.h"
#include "DOMCameraManagerObserver.h"

#define DOM_CAMERA_DEBUG_REFS 1
#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;

// observer-specific implementation of static Get()
already_AddRefed<nsDOMCameraManager>
nsDOMCameraManager::Get()
{
  // TODO: bug 776934: check for permissions here to access cameras
  if (!sCameraManager) {
    nsRefPtr<DOMCameraManagerObserver> cameraManager = new DOMCameraManagerObserver();

    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    obs->AddObserver(cameraManager, "xpcom-shutdown", false);
    obs->AddObserver(cameraManager, "inner-window-destroyed", false);

    sCameraManager = cameraManager;
  }

  nsRefPtr<nsDOMCameraManager> cameraManager = sCameraManager;
  return cameraManager.forget();
}

NS_IMPL_ISUPPORTS_INHERITED1(DOMCameraManagerObserver, nsDOMCameraManager, nsIObserver)

DOMCameraManagerObserver::DOMCameraManagerObserver()
{
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
  mActiveWindows.Init();
}

DOMCameraManagerObserver::~DOMCameraManagerObserver()
{
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
}

void
DOMCameraManagerObserver::Register(PRUint64 aWindowId, nsICameraControl* aDOMCameraControl)
{
  DOM_CAMERA_LOGI(">>> Register( aWindowId =0x%llx, aDOMCameraControl = %p )\n", aWindowId, aDOMCameraControl);

  nsRefPtr<nsDOMCameraControl> cameraControl = static_cast<nsDOMCameraControl*>(aDOMCameraControl);

  // Put the camera control into the hash table
  CameraControls* controls = GetActiveWindows()->Get(aWindowId);
  if (!controls) {
    controls = new CameraControls;
    GetActiveWindows()->Put(aWindowId, controls);
  }
  controls->AppendElement(cameraControl);
}

void
DOMCameraManagerObserver::Shutdown(PRUint64 aWindowId)
{
  DOM_CAMERA_LOGI(">>> Shutdown( aWindowId = 0x%llx )\n", aWindowId);

  CameraControls* controls = GetActiveWindows()->Get(aWindowId);
  if (!controls) {
    return;
  }

  PRUint32 length = controls->Length();
  for (PRUint32 i = 0; i < length; i++) {
    nsRefPtr<nsDOMCameraControl> cameraControl = controls->ElementAt(i);
    cameraControl->Shutdown();
  }
  controls->Clear();

  GetActiveWindows()->Remove(aWindowId);
}

nsresult
DOMCameraManagerObserver::InnerWindowDestroyed(nsISupports* aSubject)
{
  nsCOMPtr<nsISupportsPRUint64> wrapper = do_QueryInterface(aSubject);
  NS_ENSURE_TRUE(wrapper, NS_ERROR_FAILURE);

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  obs->RemoveObserver(this, "inner-window-destroyed");

  PRUint64 windowId;
  nsresult rv = wrapper->GetData(&windowId);
  NS_ENSURE_SUCCESS(rv, rv);
  DOM_CAMERA_LOGI(">>> Inner Window Destroyed (0x%llx)\n", windowId);
  
  Shutdown(windowId);
  return NS_OK;
}

#if 0
nsresult
DOMCameraManagerObserver::XpComShutdown()
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  obs->RemoveObserver(this, "xpcom-shutdown");

  mActiveWindows.Clear();
  sCameraManager = nullptr;

  return NS_OK;
}
#endif

nsresult
DOMCameraManagerObserver::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aData)
{
  if (strcmp(aTopic, "inner-window-destroyed") == 0) {
    return InnerWindowDestroyed(aSubject);
  }
#if 0
  if (strcmp(aTopic, "xpcom-shutdown") == 0) {
    return XpComShutdown();
  }
#endif
  return NS_OK;
}

void
DOMCameraManagerObserver::OnNavigation(PRUint64 aWindowId)
{
  Shutdown(aWindowId);
}

bool
DOMCameraManagerObserver::IsWindowStillActive(PRUint64 aWindowId)
{
  MOZ_ASSERT(NS_IsMainThread());
  WindowTable* activeWindows = GetActiveWindows();
  if (!activeWindows) {
    return false;
  }
  return !!activeWindows->Get(aWindowId);
}

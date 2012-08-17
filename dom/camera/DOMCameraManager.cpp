/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsJSUtils.h"
#include "nsDOMClassInfo.h"
#include "DOMCameraManager.h"
#include "DOMCameraControl.h"
#include "DictionaryHelpers.h"

#define DOM_CAMERA_DEBUG_REFS 1
#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;
using namespace dom;

DOMCI_DATA(CameraManager, nsIDOMCameraManager)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMCameraManager)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMCameraManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCameraThread)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMCameraManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCameraThread)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMCameraManager)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCameraManager)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CameraManager)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMCameraManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMCameraManager)

/**
 * nsDOMCameraManager::GetListOfCameras
 * is implementation-specific, and can be found in (e.g.)
 * GonkCameraManager.cpp and FallbackCameraManager.cpp.
 */
 
nsRefPtr<nsDOMCameraManager> nsDOMCameraManager::sCameraManager;
 
nsDOMCameraManager::nsDOMCameraManager()
  : mCameraThread(nullptr)
{
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
}

nsDOMCameraManager::~nsDOMCameraManager()
{
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
}

/* [implicit_jscontext] void getCamera ([optional] in jsval aOptions, in nsICameraGetCameraCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP
nsDOMCameraManager::GetCamera(const JS::Value& aOptions, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError, JSContext* cx)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  PRUint32 cameraId = 0;  // back (or forward-facing) camera by default
  CameraSelector selector;

  nsresult rv = selector.Init(cx, &aOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  if (selector.camera.EqualsASCII("front")) {
    cameraId = 1;
  }

  // reuse the same camera thread to conserve resources
  if (!mCameraThread) {
    rv = NS_NewThread(getter_AddRefs(mCameraThread));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
  PRUint64 windowId = nsJSUtils::GetCurrentlyRunningCodeInnerWindowID(cx);

  // Creating this object will trigger the onSuccess handler
  nsCOMPtr<nsICameraControl> cameraControl = new nsDOMCameraControl(cameraId, mCameraThread, onSuccess, onError, windowId);

  Register(windowId, cameraControl);
  return NS_OK;
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CameraControl.h"
#include "DOMCameraManager.h"
#include "nsDOMClassInfo.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

USING_CAMERA_NAMESPACE

DOMCI_DATA(CameraManager, nsIDOMCameraManager)

NS_INTERFACE_MAP_BEGIN(nsDOMCameraManager)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCameraManager)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CameraManager)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDOMCameraManager)
NS_IMPL_RELEASE(nsDOMCameraManager)

/**
 * nsDOMCameraManager::GetListOfCameras
 * is implementation-specific, and can be found in (e.g.)
 * GonkCameraManager.cpp and FallbackCameraManager.cpp.
 */

nsDOMCameraManager::nsDOMCameraManager(PRUint64 aWindowId)
  : mWindowId(aWindowId)
{
  /* member initializers and constructor code */
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
}

nsDOMCameraManager::~nsDOMCameraManager()
{
  /* destructor code */
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
}

void
nsDOMCameraManager::OnNavigation(PRUint64 aWindowId)
{
  // TODO: implement -- see getUserMedia() implementation
}

// static creator
NS_IMETHODIMP
nsDOMCameraManager::Create(PRUint64 aWindowId, nsDOMCameraManager * *aMozCameras)
{
  // TODO: check for permissions here to access cameras

  nsRefPtr<nsDOMCameraManager> cameraManager = new nsDOMCameraManager(aWindowId);
  cameraManager.forget(aMozCameras);
  return NS_OK;
}

/* [implicit_jscontext] void getCamera ([optional] in jsval aOptions, in nsICameraGetCameraCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP
nsDOMCameraManager::GetCamera(const JS::Value & aOptions, nsICameraGetCameraCallback *onSuccess, nsICameraErrorCallback *onError, JSContext* cx)
{
  nsresult rv;
  PRUint32 cameraId = 0;  // back (or forward-facing) camera by default

  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  if (aOptions.isObject()) {
    JSObject *options = JSVAL_TO_OBJECT(aOptions);
    jsval v;

    if (JS_GetProperty(cx, options, "camera", &v)) {
      if (JSVAL_IS_STRING(v)) {
        const char* camera = JS_EncodeString(cx, JSVAL_TO_STRING(v));
        if (camera) {
          if (strcmp(camera, "front") == 0) {
            cameraId = 1;
          }
        }
      }
    }
  }

  // reuse the same camera thread to conserve resources
  if (!mCameraThread) {
    rv = NS_NewThread(getter_AddRefs(mCameraThread));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

  nsCOMPtr<nsIRunnable> doGetCamera = new DoGetCamera(cameraId, onSuccess, onError, mCameraThread);
  mCameraThread->Dispatch(doGetCamera, NS_DISPATCH_NORMAL);

  return NS_OK;
}

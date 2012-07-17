/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "jsapi.h"
#include "libcameraservice/CameraHardwareInterface.h"
#include "GonkCameraControl.h"
#include "DOMCameraManager.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"


/*
  From nsDOMCameraManager, but gonk-specific!
*/

/* [implicit_jscontext] jsval getListOfCameras (); */
NS_IMETHODIMP
nsDOMCameraManager::GetListOfCameras(JSContext* cx, JS::Value *_retval NS_OUTPARAM)
{
  JSObject* a = JS_NewArrayObject(cx, 0, nsnull);
  camera_module_t* module;
  PRUint32 index = 0;
  PRUint32 count;
  char d[32];

  if (!a) {
    DOM_CAMERA_LOGE("getListOfCameras : Could not create array object");
    return NS_ERROR_OUT_OF_MEMORY;
  }
  if (hw_get_module(CAMERA_HARDWARE_MODULE_ID, (const hw_module_t **)&module) < 0) {
    DOM_CAMERA_LOGE("getListOfCameras : Could not load camera HAL module");
    return NS_ERROR_NOT_AVAILABLE;
  }

  count = module->get_number_of_cameras();
  DOM_CAMERA_LOGI("getListOfCameras : get_number_of_cameras() returned %d\n", count);
  while (count--) {
    JSString* v;
    jsval jv;

    switch (count) {
      case 0:
        v = JS_NewStringCopyZ(cx, "back");
        break;

      case 1:
        v = JS_NewStringCopyZ(cx, "front");
        break;

      default:
        /* TODO: handle extra cameras in getCamera() */
        snprintf(d, sizeof(d), "extra-camera-%d", count);
        v = JS_NewStringCopyZ(cx, d);
        break;
    }
    if (!v) {
      DOM_CAMERA_LOGE("getListOfCameras : out of memory populating camera list");
      /* TODO: clean up any partial objects? */
      return NS_ERROR_NOT_AVAILABLE;
    }
    jv = STRING_TO_JSVAL(v);
    JS_SetElement(cx, a, index++, &jv);
  }

  *_retval = OBJECT_TO_JSVAL(a);
  return NS_OK;
}

USING_CAMERA_NAMESPACE

NS_IMETHODIMP
DoGetCamera::Run()
{
  nsCOMPtr<nsICameraControl> cameraControl = new nsGonkCameraControl(mCameraId, mCameraThread);

  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);

  nsresult rv = NS_DispatchToMainThread(new GetCameraResult(cameraControl, mOnSuccessCb));
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch getCamera() onSuccess callback to main thread!");
  }
  return NS_OK;
}

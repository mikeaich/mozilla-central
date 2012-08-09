/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_DOMCameraCapabilities_H
#define DOM_CAMERA_DOMCameraCapabilities_H

#include "nsCycleCollectionParticipant.h"
#include "DOMCameraControl.h"
#include "nsAutoPtr.h"

namespace mozilla {

typedef nsresult (*ParseItemAndAddFunc)(JSContext* aCx, JSObject* aArray, PRUint32 aIndex, const char* aStart, char** aEnd);

class DOMCameraCapabilities : public nsICameraCapabilities
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMCameraCapabilities)
  NS_DECL_NSICAMERACAPABILITIES

  DOMCameraCapabilities(CameraControl* aCamera)
    : mCamera(aCamera)
  {
    DOM_CAMERA_LOGI("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  nsresult ParameterListToNewArray(
    JSContext* cx,
    JSObject** aArray,
    PRUint32 aKey,
    ParseItemAndAddFunc aParseItemAndAdd
  );
  nsresult StringListToNewObject(JSContext* aCx, JS::Value* aArray, PRUint32 aKey);
  nsresult DimensionListToNewObject(JSContext* aCx, JS::Value* aArray, PRUint32 aKey);

private:
  DOMCameraCapabilities(const DOMCameraCapabilities&) MOZ_DELETE;
  DOMCameraCapabilities& operator=(const DOMCameraCapabilities&) MOZ_DELETE;

protected:
  /* additional members */
  ~DOMCameraCapabilities()
  {
    // destructor code
    DOM_CAMERA_LOGI("%s:%d : this=%p, mCamera=%p\n", __func__, __LINE__, this, mCamera.get());
  }

  nsRefPtr<CameraControl> mCamera;
};

} // namespace mozilla

#endif // DOM_CAMERA_DOMCameraCapabilities_H

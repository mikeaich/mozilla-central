/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_NSCAMERACAPABILITIES_H
#define DOM_CAMERA_NSCAMERACAPABILITIES_H

#include "nsCycleCollectionParticipant.h"
#include "CameraControl.h"
#include "nsAutoPtr.h"

namespace mozilla {

class nsCameraCapabilities : public nsICameraCapabilities
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsCameraCapabilities)
  NS_DECL_NSICAMERACAPABILITIES

  nsCameraCapabilities(nsCameraControl *aCamera);

  nsresult parameterListToNewArray(
    JSContext *cx,
    JSObject **array,
    const char *key,
    nsresult (*parseItemAndAdd)(
      JSContext *cx,
      JSObject *array,
      PRUint32 index,
      const char *start,
      char **end
    )
  );
  nsresult stringListToNewObject(JSContext* cx, JS::Value *aArray, const char *key);
  nsresult dimensionListToNewObject(JSContext* cx, JS::Value *aArray, const char *key);

private:
  nsCameraCapabilities(const nsCameraCapabilities&);
  nsCameraCapabilities& operator=(const nsCameraCapabilities&);
  ~nsCameraCapabilities();

protected:
  /* additional members */
  nsCOMPtr<nsCameraControl> mCamera;
};

} // namespace mozilla

#endif // DOM_CAMERA_NSCAMERACAPABILITIES_H

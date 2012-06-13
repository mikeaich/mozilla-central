/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_NSCAMERACAPABILITIES_H
#define DOM_CAMERA_NSCAMERACAPABILITIES_H


#include "nsIDOMCameraManager.h"
#include "nsCOMPtr.h"


class nsCameraCapabilities : public nsICameraCapabilities
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICAMERACAPABILITIES

  nsCameraCapabilities(nsCOMPtr<nsIDOMCameraManager> aCamera);

private:
  ~nsCameraCapabilities();

protected:
  /* additional members */
  nsCOMPtr<nsIDOMCameraManager> mCamera;
};


#endif // DOM_CAMERA_NSCAMERACAPABILITIES_H

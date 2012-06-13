/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_NSDOMNAVIGATORCAMERA_H
#define DOM_CAMERA_NSDOMNAVIGATORCAMERA_H


#include "nsIDOMNavigatorCamera.h"


class nsDOMNavigatorCamera : public nsIDOMNavigatorCamera
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMNAVIGATORCAMERA

  nsDOMNavigatorCamera();

private:
  ~nsDOMNavigatorCamera();

protected:
  /* additional members */
};


#endif // DOM_CAMERA_NSDOMNAVIGATORCAMERA_H

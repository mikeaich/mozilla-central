/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_DOMCAMERAMANAGEROBSERVER_H
#define DOM_CAMERA_DOMCAMERAMANAGEROBSERVER_H

#include "nsHashKeys.h"
#include "nsClassHashtable.h"
#include "DOMCameraControl.h"
#include "DOMCameraManager.h"

typedef nsTArray<nsRefPtr<nsDOMCameraControl> > CameraControls;
typedef nsClassHashtable<nsUint64HashKey, CameraControls> WindowTable;

class DOMCameraManagerObserver
  : public nsDOMCameraManager
  , public nsIObserver
{
public:
  NS_DECL_NSIOBSERVER
  NS_DECL_ISUPPORTS_INHERITED

  DOMCameraManagerObserver();

  WindowTable* GetActiveWindows()
  {
    return &mActiveWindows;
  }

  void Register(PRUint64 aWindowId, nsICameraControl* aDOMCameraControl);
  void Shutdown(PRUint64 aWindowId);
  nsresult InnerWindowDestroyed(nsISupports* aSubject);
  nsresult XpComShutdown();
  void OnNavigation(PRUint64 aWindowId);
  bool IsWindowStillActive(PRUint64 aWindowId);

protected:
  virtual ~DOMCameraManagerObserver();

private:
  DOMCameraManagerObserver(const DOMCameraManagerObserver&) MOZ_DELETE;
  DOMCameraManagerObserver& operator=(const DOMCameraManagerObserver&) MOZ_DELETE;

protected:
  WindowTable mActiveWindows;
  static nsRefPtr<nsDOMCameraManager> sCameraManager;
};

#endif // DOM_CAMERA_DOMCAMERAMANAGEROBSERVER_H

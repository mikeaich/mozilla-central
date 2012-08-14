/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_ICAMERACONTROL_H
#define DOM_CAMERA_ICAMERACONTROL_H

#include "prtypes.h"
#include "jsapi.h"
#include "nsIDOMCameraManager.h"
#include "DictionaryHelpers.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

namespace mozilla {
  
using namespace dom;

class DOMCameraPreview;

class ICameraControl
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ICameraControl)

  virtual nsresult GetPreviewStream(CameraSize aSize, nsICameraPreviewStreamCallback* onSuccess, nsICameraErrorCallback* onError) = 0;
  virtual nsresult StartPreview(DOMCameraPreview* aDOMPreview) = 0;
  virtual void StopPreview() = 0;
  virtual nsresult AutoFocus(nsICameraAutoFocusCallback* onSuccess, nsICameraErrorCallback* onError) = 0;
  virtual nsresult TakePicture(CameraSize aSize, PRInt32 aRotation, const nsAString& aFileFormat, CameraPosition aPosition, nsICameraTakePictureCallback* onSuccess, nsICameraErrorCallback* onError) = 0;
  virtual nsresult StartRecording(CameraSize aSize, nsICameraStartRecordingCallback* onSuccess, nsICameraErrorCallback* onError) = 0;
  virtual nsresult StopRecording() = 0;

  virtual nsresult Set(PRUint32 aKey, const nsAString& aValue) = 0;
  virtual nsresult Get(PRUint32 aKey, nsAString& aValue) = 0;
  virtual nsresult Set(PRUint32 aKey, double aValue) = 0;
  virtual nsresult Get(PRUint32 aKey, double* aValue) = 0;
  virtual nsresult Set(JSContext* aCx, PRUint32 aKey, const JS::Value& aValue, PRUint32 aLimit) = 0;
  virtual nsresult Get(JSContext* aCx, PRUint32 aKey, JS::Value* aValue) = 0;
  virtual nsresult Set(nsICameraShutterCallback* aOnShutter) = 0;
  virtual nsresult Get(nsICameraShutterCallback** aOnShutter) = 0;
  virtual nsresult SetFocusAreas(JSContext* aCx, const JS::Value& aValue) = 0;
  virtual nsresult SetMeteringAreas(JSContext* aCx, const JS::Value& aValue) = 0;

  virtual const char* GetParameter(const char* aKey) = 0;
  virtual const char* GetParameterConstChar(PRUint32 aKey) = 0;
  virtual double GetParameterDouble(PRUint32 aKey) = 0;
  virtual void GetParameter(PRUint32 aKey, nsTArray<CameraRegion>& aRegions) = 0;
  virtual void SetParameter(const char* aKey, const char* aValue) = 0;
  virtual void SetParameter(PRUint32 aKey, const char* aValue) = 0;
  virtual void SetParameter(PRUint32 aKey, double aValue) = 0;
  virtual void SetParameter(PRUint32 aKey, const nsTArray<CameraRegion>& aRegions) = 0;

protected:
  virtual ~ICameraControl() { }
};

} // namespace mozilla

#endif // DOM_CAMERA_ICAMERACONTROL_H

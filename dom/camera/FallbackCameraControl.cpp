/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CameraControlImpl.h"

using namespace mozilla;

/**
 * Fallback camera control subclass.  Can be used as a template for the
 * definition of new camera support classes.
 */
class nsFallbackCameraControl : public CameraControlImpl
{
public:
  nsFallbackCameraControl(PRUint32 aCameraId, nsIThread* aCameraThread);

  const char* GetParameter(const char* aKey);
  const char* GetParameterConstChar(PRUint32 aKey);
  double GetParameterDouble(PRUint32 aKey);
  void GetParameter(PRUint32 aKey, nsTArray<dom::CameraRegion>& aRegions);
  void SetParameter(const char* aKey, const char* aValue);
  void SetParameter(PRUint32 aKey, const char* aValue);
  void SetParameter(PRUint32 aKey, double aValue);
  void SetParameter(PRUint32 aKey, const nsTArray<dom::CameraRegion>& aRegions);
  nsresult PushParameters();

protected:
  ~nsFallbackCameraControl();

  nsresult GetPreviewStreamImpl(GetPreviewStreamTask* aGetPreviewStream);
  nsresult StartPreviewImpl(StartPreviewTask* aStartPreview);
  nsresult StopPreviewImpl(StopPreviewTask* aStopPreview);
  nsresult AutoFocusImpl(AutoFocusTask* aAutoFocus);
  nsresult TakePictureImpl(TakePictureTask* aTakePicture);
  nsresult StartRecordingImpl(StartRecordingTask* aStartRecording);
  nsresult StopRecordingImpl(StopRecordingTask* aStopRecording);
  nsresult PushParametersImpl();
  nsresult PullParametersImpl();

private:
  nsFallbackCameraControl(const nsFallbackCameraControl&) MOZ_DELETE;
  nsFallbackCameraControl& operator=(const nsFallbackCameraControl&) MOZ_DELETE;
};

/**
 * Stub implementation of the DOM-facing camera control constructor.
 *
 * This should never get called--it exists to keep the linker happy; if
 * implemented, it should construct (e.g.) nsFallbackCameraControl and
 * store a reference in the 'mCameraControl' member (which is why it is
 * defined here).
 */
nsDOMCameraControl::nsDOMCameraControl(PRUint32 aCameraId, nsIThread* aCameraThread, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError)
  : mDOMCapabilities(nullptr)
{
}

/**
 * Stub implemetations of the fallback camera control.
 *
 * None of these should ever get called--they exist to keep the linker happy,
 * and may be used as templates for new camera support classes.
 */
nsFallbackCameraControl::nsFallbackCameraControl(PRUint32 aCameraId, nsIThread* aCameraThread)
  : nsCameraControl(aCameraId, aCameraThread)
{
}

nsFallbackCameraControl::~nsFallbackCameraControl()
{
}

const char*
nsFallbackCameraControl::GetParameter(const char* aKey)
{
  return nullptr;
}

const char*
nsFallbackCameraControl::GetParameterConstChar(PRUint32 aKey)
{
  return nullptr;
}

double
nsFallbackCameraControl::GetParameterDouble(PRUint32 aKey)
{
  return NAN;
}

void
nsFallbackCameraControl::GetParameter(PRUint32 aKey, nsTArray<dom::CameraRegion>& aRegions)
{
}

void
nsFallbackCameraControl::SetParameter(const char* aKey, const char* aValue)
{
}

void
nsFallbackCameraControl::SetParameter(PRUint32 aKey, const char* aValue)
{
}

void
nsFallbackCameraControl::SetParameter(PRUint32 aKey, double aValue)
{
}

void
nsFallbackCameraControl::SetParameter(PRUint32 aKey, const nsTArray<dom::CameraRegion>& aRegions)
{
}

nsresult
nsFallbackCameraControl::PushParameters()
{
}

nsresult
nsFallbackCameraControl::GetPreviewStreamImpl(GetPreviewStreamTask* aGetPreviewStream)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::StartPreviewImpl(StartPreviewTask* aGetPreviewStream)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::StopPreviewImpl(StopPreviewTask* aGetPreviewStream)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::AutoFocusImpl(AutoFocusTask* aAutoFocus)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::TakePictureImpl(TakePictureTask* aTakePicture)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::StartRecordingImpl(StartRecordingTask* aStartRecording)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::StopRecordingImpl(StopRecordingTask* aStopRecording)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::PushParametersImpl()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::PullParametersImpl()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

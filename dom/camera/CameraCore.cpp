/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CameraCore.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;

void
CameraCore::Shutdown()
{
  // TODO: fill this in if required, remove if not.
}

nsresult
CameraCore::GetPreviewStream(CameraSize aSize, nsICameraPreviewStreamCallback* onSuccess, nsICameraErrorCallback* onError)
{
  nsCOMPtr<nsIRunnable> getPreviewStreamTask = new GetPreviewStreamTask(this, size, onSuccess, onError);
  return NS_DispatchToMainThread(getPreviewStreamTask);
}

nsresult
CameraCore::AutoFocus(nsICameraAutoFocusCallback* onSuccess, nsICameraErrorCallback* onError)
{
  nsCOMPtr<nsIRunnable> autoFocusTask = new AutoFocusTask(this, onSuccess, onError);
  return mCameraThread->Dispatch(autoFocusTask, NS_DISPATCH_NORMAL);
}

nsresult
CameraCore::TakePicture(CameraSize aSize, PRInt32 aRotation, const nsAString& aFileFormat, CameraPosition aPosition, nsICameraTakePictureCallback* onSuccess, nsICameraErrorCallback* onError)
{
  nsCOMPtr<nsIRunnable> takePictureTask = new TakePictureTask(this, size, options.rotation, options.fileFormat, pos, onSuccess, onError);
  return mCameraThread->Dispatch(takePictureTask, NS_DISPATCH_NORMAL);
}

nsresult
CameraCore::StartRecording(CameraSize aSize, nsICameraStartRecordingCallback* onSuccess, nsICameraErrorCallback* onError)
{
  nsCOMPtr<nsIRunnable> startRecordingTask = new StartRecordingTask(this, size, onSuccess, onError);
  return mCameraThread->Dispatch(startRecordingTask, NS_DISPATCH_NORMAL);
}

nsresult
CameraCore::StopRecording()
{
  nsCOMPtr<nsIRunnable> stopRecordingTask = new StopRecordingTask(this);
  return mCameraThread->Dispatch(stopRecordingTask, NS_DISPATCH_NORMAL);
}

nsresult
CameraCore::PushParameters()
{

}

nsresult
CameraCore::PullParameters()
{

}

void
CameraCore::AutoFocusComplete(bool aSuccess)
{
  /**
   * Auto focusing can change some of the camera's parameters, so
   * we need to pull a new set before sending the result to the
   * main thread.
   */
  PullParametersImpl(nullptr);

  nsCOMPtr<nsIRunnable> autoFocusResult = new AutoFocusResult(aSuccess, mAutoFocusOnSuccessCb);

  nsresult rv = NS_DispatchToMainThread(autoFocusResult);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch autoFocus() onSuccess callback to main thread!");
  }
}

void
CameraCore::TakePictureComplete(PRUint8* aData, PRUint32 aLength)
{
  PRUint8* data = new PRUint8[aLength];

  memcpy(data, aData, aLength);

  /**
   * TODO: pick up the actual specified picture format for the MIME type;
   * for now, assume we'll be using JPEGs.
   */
  nsIDOMBlob* blob = new nsDOMMemoryFile(static_cast<void*>(data), static_cast<PRUint64>(aLength), NS_LITERAL_STRING("image/jpeg"));
  nsCOMPtr<nsIRunnable> takePictureResult = new TakePictureResult(blob, mTakePictureOnSuccessCb);

  nsresult rv = NS_DispatchToMainThread(takePictureResult);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch takePicture() onSuccess callback to main thread!");
  }
}

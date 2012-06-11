/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=40: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_camera_cameramanager_h__
#define mozilla_dom_camera_cameramanager_h__


#include "CameraCommon.h"
#include "nsIDOMCameraManager.h"
#include "nsWeakReference.h"
#include "mozilla/Services.h"
#include "mozilla/Observer.h"
#include "nsHashKeys.h"
#include "nsClassHashtable.h"
#include "nsObserverService.h"

#include "nsIDOMNavigatorCamera.h"


BEGIN_CAMERA_NAMESPACE

class GetCameraCallbackMediaStreamListener : public MediaStreamListener
{
public:
  GetCameraCallbackMediaStreamListener(MediaEngineSource* aSource,
    nsDOMMediaStream* aStream,
    TrackID aListenId)
      : mSource(aSource)
      , mStream(aStream)
      , mId(aListenId)
      , mValid(true) { }

  void
  Invalidate()
  {
    if (!mValid) {
      return;
    }

    mValid = false;
    /*
    mSource->Stop();
    mSource->Deallocate();
    */
  }
  
  void
  NotifyConsumptionChanged(MediaStreamGraph* aGraph, Consumption aComsuming)
  {
    if (aConsuming == CONSUMED) {
      nsRefPtr<SourceMediaStream> stream = mStream->GetStream()->AsSourceStream();
      mSource->Start(stream, mId);
      return;
    }

    // NOT_CONSUMED
    Invalidate();
    return;
  }

  void NotifyBlockingChanged(MediaStreamGraph* aGraph, Blocking aBlocked) { }
  void NotifyOutput(MediaStreamGraph* aGraph) { }
  void NotifyFinished(MediaStreamGraph* aGraph) { }
  void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
    TrackRate aTrackRate, TrackTicks aTrackOffset,
    PRUint32 aTrackEvents, const MediaSegment& aQueuedMedia) { }
  nsresult Run() { return NS_OK; }

private:
  nsCOMPtr<MediaEngineSource> mSource;
  nsCOMPtr<nsDOMMediaStream> mStream;
  TrackID mId;
  bool mValid;
};


typedef nsTArray<nsRefPtr<GetCameraCallbackMediaStreamListener> > PreviewListeners;
typedef nsClassHashtable<nsUint64HashKey, PreviewListeners> WindowTable;

class CameraManager : public nsIObserver
{
public:
  static CameraManager* Get()
  {
    if (!sSingleton) {
      sSingleton = new CameraManager();
        
      nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
      obs->AddObserver(sSingleton, "xpcom-shutdown", false);
    }
    return sSingleton;
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  Mutex* GetLock()
  {
    return mLock;
  }
  
  WindowTable* GetActiveWindows();
  void OnNavigation(PRUint64 aWindowId);
  
private:
  // private to enforce singleton-ness
  CameraManager() : mCameraThread(nsnull)
  {
    mLock = new Mutex("CameraManager::PreviewListenersLock");
    mAvtiveWindows.Init();
  }
  CameraManager(CameraManager const&) { };

  ~CameraManager()
  {
    delete mLock;
  }

  nsCOMPtr<nsIThread> mCameraThread;
  Mutex* mLock;
  WindowTable mActiveWindows;

  static nsRefPtr<CameraManager> sSingleton;
};

END_CAMERA_NAMESPACE


nsresult NS_NewCameraManager(PRUint64 aWindowId, nsIDOMCameraManager** aCameraManager);


#endif

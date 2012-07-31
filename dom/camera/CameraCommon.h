/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=40: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_CAMERACOMMON_H
#define DOM_CAMERA_CAMERACOMMON_H

#ifndef __func__
#ifdef __FUNCTION__
#define __func__ __FUNCTION__
#else
#define __func__ __FILE__
#endif
#endif

#ifndef NAN
#define NAN std::numeric_limits<double>::quiet_NaN()
#endif

#include "nsThreadUtils.h"
#include "nsIDOMCameraManager.h"

#define DOM_CAMERA_LOG( l, ... )          \
  do {                                    \
    if ( DOM_CAMERA_LOG_LEVEL >= (l) ) {  \
      printf_stderr (__VA_ARGS__);        \
    }                                     \
  } while (0)

#define DOM_CAMERA_LOGA( ... )        DOM_CAMERA_LOG( 0, __VA_ARGS__ )

enum {
  DOM_CAMERA_LOG_NOTHING,
  DOM_CAMERA_LOG_ERROR,
  DOM_CAMERA_LOG_WARNING,
  DOM_CAMERA_LOG_INFO
};

#define DOM_CAMERA_LOGI( ... )        DOM_CAMERA_LOG( DOM_CAMERA_LOG_INFO,  __VA_ARGS__ )
#define DOM_CAMERA_LOGW( ... )        DOM_CAMERA_LOG( DOM_CAMERA_LOG_WARNING, __VA_ARGS__ )
#define DOM_CAMERA_LOGE( ... )        DOM_CAMERA_LOG( DOM_CAMERA_LOG_ERROR, __VA_ARGS__ )

class CameraErrorResult : public nsRunnable
{
public:
  CameraErrorResult(nsICameraErrorCallback* onError, const nsString& aErrorMsg)
    : mOnErrorCb(onError)
    , mErrorMsg(aErrorMsg)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnErrorCb) {
      mOnErrorCb->HandleEvent(mErrorMsg);
    }
    return NS_OK;
  }

protected:
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
  const nsString mErrorMsg;
};

#endif // DOM_CAMERA_CAMERACOMMON_H

#if DOM_CAMERA_DEBUG_REFS
#ifdef NS_LOG_ADDREF
#undef NS_LOG_ADDREF
#endif
#ifdef NS_LOG_RELEASE
#undef NS_LOG_RELEASE
#endif

static inline void nsLogAddRefCamera(const char *file, PRUint32 line, void* p, PRUint32 count, const char *clazz, PRUint32 size)
{
  if (count == 1) {
    DOM_CAMERA_LOGI("++++++++++++++++++++++++++++++++++++++++");
  }
  DOM_CAMERA_LOGI("%s:%d : CAMREF-ADD(%s): this=%p, mRefCnt=%d\n", file, line, clazz, p, count);
}

static inline void nsLogReleaseCamera(const char *file, PRUint32 line, void* p, PRUint32 count, const char *clazz, bool abortOnDelete)
{
  DOM_CAMERA_LOGI("%s:%d : CAMREF-REL(%s): this=%p, mRefCnt=%d\n", file, line, clazz, p, count);
  if (count == 0) {
    DOM_CAMERA_LOGI("----------------------------------------");
    if (abortOnDelete) {
      DOM_CAMERA_LOGI("---------- ABORTING ON DELETE ----------");
      *((PRUint32 *)0xdeadbeef) = 0x266230;
    }
  }
}

#define NS_LOG_ADDREF( p, n, c, s ) nsLogAddRefCamera(__FILE__, __LINE__, (p), (n), (c), (s))
#ifdef DOM_CAMERA_DEBUG_REFS_ABORT_ON_DELETE
#define NS_LOG_RELEASE( p, n, c )   nsLogReleaseCamera(__FILE__, __LINE__, (p), (n), (c), DOM_CAMERA_DEBUG_REFS_ABORT_ON_DELETE)
#else
#define NS_LOG_RELEASE( p, n, c )   nsLogReleaseCamera(__FILE__, __LINE__, (p), (n), (c), false)
#endif
#endif

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CameraSupport.h"
#include "nsIProgrammingLanguage.h"
#include "nsIClassInfoImpl.h"
#include "nsMemory.h"
#include "CameraImpl.h"

NS_IMPL_ISUPPORTS2(CameraSupport, nsICameraSupport, nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER1(CameraSupport, nsICameraSupport)

static NS_DEFINE_CID(kCameraSupportCID, CameraSupport_CID);

CameraSupport::CameraSupport()
{
  /* member initializers and constructor code */
}

CameraSupport::~CameraSupport()
{
  /* destructor code */
}

/* [implicit_jscontext] void getCamera([optional] in jsval aOptions, in nsICameraGetCameraCallback onSuccess, [optional] in nsICameraErrorCallback onError); */
NS_IMETHODIMP CameraSupport::GetCamera(const JS::Value & aOptions, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError, JSContext* cx)
{
  return CameraImpl::GetCamera(aOptions, onSuccess, onError, cx);
}

/* [implicit_jscontext] jsval getListOfCameras (); */
NS_IMETHODIMP CameraSupport::GetListOfCameras(JSContext* cx, JS::Value *_retval NS_OUTPARAM)
{
  return CameraImpl::GetListOfCameras(cx, _retval);
}

// nsIClassInfo implementation

/* void getInterfaces (out PRUint32 count, [array, size_is (count), retval] out nsIIDPtr array); */
NS_IMETHODIMP CameraSupport::GetInterfaces(PRUint32 *count NS_OUTPARAM, nsIID ***array NS_OUTPARAM)
{
    return NS_CI_INTERFACE_GETTER_NAME(CameraSupport)(count, array);
}

/* nsISupports getHelperForLanguage (in PRUint32 language); */
NS_IMETHODIMP CameraSupport::GetHelperForLanguage(PRUint32 language, nsISupports * *_retval NS_OUTPARAM)
{
  *_retval = nsnull;
  return NS_OK;
}

/* readonly attribute string contractID; */
NS_IMETHODIMP CameraSupport::GetContractID(char * *aContractID)
{
  *aContractID = CameraSupport_ContractID;
  return NS_OK;
}

/* readonly attribute string classDescription; */
NS_IMETHODIMP CameraSupport::GetClassDescription(char * *aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

/* readonly attribute nsCIDPtr classID; */
NS_IMETHODIMP CameraSupport::GetClassID(nsCID **aClassID)
{
  *aClassID = (nsCID*) nsMemory::Alloc(sizeof(nsCID));
  if (!*aClassID)
      return NS_ERROR_OUT_OF_MEMORY;
  return GetClassIDNoAlloc(*aClassID);
}

/* readonly attribute PRUint32 implementationLanguage; */
NS_IMETHODIMP CameraSupport::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

/* readonly attribute PRUint32 flags; */
NS_IMETHODIMP CameraSupport::GetFlags(PRUint32 *aFlags)
{
  *aFlags = nsIClassInfo::DOM_OBJECT;
  return NS_OK;
}

/* [notxpcom] readonly attribute nsCID classIDNoAlloc; */
NS_IMETHODIMP CameraSupport::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
  *aClassIDNoAlloc = kCameraSupportCID;
  return NS_OK;
}

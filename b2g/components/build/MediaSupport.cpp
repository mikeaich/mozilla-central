/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MediaSupport.h"
#include "nsIProgrammingLanguage.h"
#include "nsIClassInfoImpl.h"
#include "nsMemory.h"
#include "MediaImpl.h"

NS_IMPL_ISUPPORTS2(MediaSupport, nsIB2GMediaSupport, nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER1(MediaSupport, nsIB2GMediaSupport)

static NS_DEFINE_CID(kMediaSupportCID, MediaSupport_CID);

MediaSupport::MediaSupport()
{
  /* member initializers and constructor code */
}

MediaSupport::~MediaSupport()
{
  /* destructor code */
}

/* [implicit_jscontext] nsIB2GCameraStream getCameraStream ([optional] in jsval aOptions); */
NS_IMETHODIMP MediaSupport::GetCameraStream(const JS::Value & aOptions, JSContext* cx, nsIDOMMediaStream * *_retval NS_OUTPARAM)
{
  return MediaImpl::GetCameraStream(aOptions, cx, _retval);
}

/* [implicit_jscontext] long getNumberOfCameras (); */
NS_IMETHODIMP MediaSupport::GetNumberOfCameras(JSContext* cx, PRUint32 *_retval NS_OUTPARAM)
{
  *_retval = MediaImpl::getNumberOfCameras();
  return NS_OK;
}

// nsIClassInfo implementation

/* void getInterfaces (out PRUint32 count, [array, size_is (count), retval] out nsIIDPtr array); */
NS_IMETHODIMP MediaSupport::GetInterfaces(PRUint32 *count NS_OUTPARAM, nsIID ***array NS_OUTPARAM)
{
    return NS_CI_INTERFACE_GETTER_NAME(MediaSupport)(count, array);
}

/* nsISupports getHelperForLanguage (in PRUint32 language); */
NS_IMETHODIMP MediaSupport::GetHelperForLanguage(PRUint32 language, nsISupports * *_retval NS_OUTPARAM)
{
  *_retval = nsnull;
  return NS_OK;
}

/* readonly attribute string contractID; */
NS_IMETHODIMP MediaSupport::GetContractID(char * *aContractID)
{
  *aContractID = MediaSupport_ContractID;
  return NS_OK;
}

/* readonly attribute string classDescription; */
NS_IMETHODIMP MediaSupport::GetClassDescription(char * *aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

/* readonly attribute nsCIDPtr classID; */
NS_IMETHODIMP MediaSupport::GetClassID(nsCID **aClassID)
{
  *aClassID = (nsCID*) nsMemory::Alloc(sizeof(nsCID));
  if (!*aClassID)
      return NS_ERROR_OUT_OF_MEMORY;
  return GetClassIDNoAlloc(*aClassID);
}

/* readonly attribute PRUint32 implementationLanguage; */
NS_IMETHODIMP MediaSupport::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

/* readonly attribute PRUint32 flags; */
NS_IMETHODIMP MediaSupport::GetFlags(PRUint32 *aFlags)
{
  *aFlags = nsIClassInfo::DOM_OBJECT;
  return NS_OK;
}

/* [notxpcom] readonly attribute nsCID classIDNoAlloc; */
NS_IMETHODIMP MediaSupport::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
  *aClassIDNoAlloc = kMediaSupportCID;
  return NS_OK;
}

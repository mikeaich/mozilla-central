/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef B2G_BASECAMERASTREAM_H
#define B2G_BASECAMERASTREAM_H

#include "nsComponentManagerUtils.h"
#include "nsIClassInfo.h"
#include "nsIProgrammingLanguage.h"
#include "nsIClassInfoImpl.h"

#define BaseCameraStream_CID \
{0xd25ef1ca, 0x630c, 0x44a9, {0x9e, 0xab, 0x03, 0x0f, 0xd6, 0xe4, 0x5b, 0x43}}

static NS_DEFINE_CID(kBaseCameraStreamCID, BaseCameraStream_CID);

#define BaseCameraStream_ContractID "@mozilla.org/b2g/camera;1"

NS_IMPL_CI_INTERFACE_GETTER2(BaseCameraStream, nsIDOMMediaStream, nsICameraControl)

class BaseCameraStream : public nsIClassInfo
{
public:
  // nsIClassInfo implementation

  /* void getInterfaces (out PRUint32 count, [array, size_is (count), retval] out nsIIDPtr array); */
  NS_IMETHODIMP
  GetInterfaces(PRUint32 *count NS_OUTPARAM, nsIID ***array NS_OUTPARAM)
  {
      return NS_CI_INTERFACE_GETTER_NAME(BaseCameraStream)(count, array);
  }

  /* nsISupports getHelperForLanguage (in PRUint32 language); */
  NS_IMETHODIMP
  GetHelperForLanguage(PRUint32 language, nsISupports * *_retval NS_OUTPARAM)
  {
    *_retval = nsnull;
    return NS_OK;
  }

  /* readonly attribute string contractID; */
  NS_IMETHODIMP
  GetContractID(char * *aContractID)
  {
    *aContractID = BaseCameraStream_ContractID;
    return NS_OK;
  }

  /* readonly attribute string classDescription; */
  NS_IMETHODIMP
  GetClassDescription(char * *aClassDescription)
  {
    *aClassDescription = nsnull;
    return NS_OK;
  }

  /* readonly attribute nsCIDPtr classID; */
  NS_IMETHODIMP
  GetClassID(nsCID **aClassID)
  {
    *aClassID = (nsCID*) nsMemory::Alloc(sizeof(nsCID));
    if (!*aClassID)
        return NS_ERROR_OUT_OF_MEMORY;
    return GetClassIDNoAlloc(*aClassID);
  }

  /* readonly attribute PRUint32 implementationLanguage; */
  NS_IMETHODIMP
  GetImplementationLanguage(PRUint32 *aImplementationLanguage)
  {
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
  }

  /* readonly attribute PRUint32 flags; */
  NS_IMETHODIMP
  GetFlags(PRUint32 *aFlags)
  {
    *aFlags = nsIClassInfo::DOM_OBJECT;
    return NS_OK;
  }

  /* [notxpcom] readonly attribute nsCID classIDNoAlloc; */
  NS_IMETHODIMP
  GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
  {
    *aClassIDNoAlloc = kBaseCameraStreamCID;
    return NS_OK;
  }
};

#endif

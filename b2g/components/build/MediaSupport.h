/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef B2G_MEDIASUPPORT_H
#define B2G_MEDIASUPPORT_H

#include "b2gmedia.h"
#include "nsIClassInfo.h"

class MediaSupport : public nsIB2GMediaSupport
                   , public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIB2GMEDIASUPPORT
  NS_DECL_NSICLASSINFO

  MediaSupport();

private:
  ~MediaSupport();

};

#define MediaSupport_CID \
{0x1b5bbf78, 0xa5e1, 0x4a75, {0x99, 0x3e, 0x8a, 0x7e, 0xfc, 0xa7, 0x76, 0x6e}}

#define MediaSupport_ContractID "@mozilla.org/b2g/media-support;1"

#endif

/* Copyright (c) 2014-2019, Fengping Bao <jamol@live.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include "PMCE_Base.h"
#include "compr/compr.h"

WS_NS_BEGIN

const std::string kPerMessageDeflate = "permessage-deflate";

class PMCE_Deflate : public PMCE_Base
{
public:
    using DataBuffer = std::vector<uint8_t>;
    PMCE_Deflate();
    ~PMCE_Deflate();
    
    KMError init();
    KMError handleIncomingFrame(FrameHeader hdr, KMBuffer &payload) override;
    KMError handleOutgoingFrame(FrameHeader hdr, KMBuffer &payload) override;
    KMError getOffer(std::string &offer) override;
    KMError negotiateAnswer(const std::string &answer) override;
    KMError negotiateOffer(const std::string &offer, std::string &answer) override;
    
    std::string getExtensionName() const override { return kPerMessageDeflate; }
    
protected:
    bool        negotiated_ = false;
    
    int         c_max_window_bits = 15;
    bool        c_no_context_takeover = false;
    DataBuffer  c_payload;
    
    int         d_max_window_bits = 15;
    DataBuffer  d_payload;
    
    std::unique_ptr<kuma::Compressor> compressor_;
    std::unique_ptr<kuma::Decompressor> decompressor_;
};

WS_NS_END


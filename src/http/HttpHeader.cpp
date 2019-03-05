/* Copyright © 2017, Fengping Bao <jamol@live.com>
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

#include "HttpHeader.h"
#include <sstream>

using namespace kuma;

KMError HttpHeader::addHeader(std::string name, std::string value)
{
    if(name.empty()) {
        return KMError::INVALID_PARAM;
    }
    
    if (is_equal(name, strContentLength)) {
        has_content_length_ = true;
        content_length_ = std::stol(value);
        if (is_outgoing_ && is_chunked_) {
            return KMError::NOERR;
        }
    } else if (is_equal(name, strTransferEncoding)) {
        std::string encoding_type;
        for_each_token(value, ',', [this, &encoding_type] (const std::string &str) {
            if (!is_equal(str, strChunked)) {
                encoding_type = str;
                return false;
            }
            return true;
        });
        if (!encoding_type.empty()) {
            if (is_outgoing_ && has_content_length_ && content_length_ == 0) {
                // no body data
                return KMError::INVALID_PARAM;
            }
            if (is_equal(encoding_type, "gzip") || is_equal(encoding_type, "deflate")) {
                encoding_type_ = std::move(encoding_type);
            } else if (is_outgoing_) {
                return KMError::NOT_SUPPORTED;
            }
        }
        
        is_chunked_ = true;
        if (is_outgoing_ && has_content_length_) {
            removeHeader(strContentLength);
        }
    } else if (is_equal(name, strContentEncoding)) {
        if (is_outgoing_ && has_content_length_ && content_length_ == 0) {
            // no body
            return KMError::INVALID_PARAM;
        }
        if (is_equal(value, "gzip") || is_equal(value, "deflate")) {
            encoding_type_ = value;
        } else if (is_outgoing_) {
            return KMError::NOT_SUPPORTED;
        }
        if (is_outgoing_ && !is_chunked_) {
            HttpHeader::addHeader(strTransferEncoding, strChunked);
        }
    }
    
    header_vec_.emplace_back(std::move(name), std::move(value));
    
    return KMError::NOERR;
}

KMError HttpHeader::addHeader(std::string name, uint32_t value)
{
    return addHeader(std::move(name), std::to_string(value));
}

bool HttpHeader::removeHeader(const std::string &name)
{
    if (is_equal(name, strContentEncoding)) {
        encoding_type_.clear();
    }
    
    bool removed = false;
    auto it = header_vec_.begin();
    while (it != header_vec_.end()) {
        if (is_equal(it->first, name)) {
            it = header_vec_.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }
    
    return removed;
}

bool HttpHeader::removeHeaderValue(const std::string &name, const std::string &value)
{
    bool removed = false;
    auto it = header_vec_.begin();
    while (it != header_vec_.end()) {
        bool erased = false;
        if (is_equal(it->first, name)) {
            if (remove_token(it->second, value, ',')) {
                removed = true;
                if (it->second.empty()) {
                    it = header_vec_.erase(it);
                    erased = true;
                }
            }
        }
        if (!erased) {
            ++it;
        }
    }
    
    return removed;
}

bool HttpHeader::hasHeader(const std::string &name) const
{
    for (auto const &kv : header_vec_) {
        if (is_equal(kv.first, name)) {
            return true;
        }
    }
    return false;
}

const std::string& HttpHeader::getHeader(const std::string &name) const
{
    for (auto const &kv : header_vec_) {
        if (is_equal(kv.first, name)) {
            return kv.second;
        }
    }
    return EmptyString;
}

void HttpHeader::processHeader()
{
    has_body_ = is_chunked_ || (has_content_length_ && content_length_ > 0);
}

void HttpHeader::processHeader(int status_code, const std::string &req_method)
{
    if (is_equal(req_method, "HEAD")) {
        has_body_ = false;
        return;
    }
    if (is_equal(req_method, "CONNECT") && status_code >= 200 && status_code <= 299) {
        has_body_ = false;
        return;
    }
    if ((status_code >= 100 && status_code <= 199) || status_code == 204 || status_code == 304) {
        has_body_ = false;
        return;
    }
    has_body_ = !has_content_length_ || content_length_ > 0;
}

std::string HttpHeader::buildHeader(const std::string &method, const std::string &url, const std::string &ver)
{
    processHeader();
    std::string req = method + " " + url + " " + (!ver.empty()?ver:VersionHTTP1_1);
    req += "\r\n";
    for (auto &kv : header_vec_) {
        req += kv.first + ": " + kv.second + "\r\n";
    }
    req += "\r\n";
    return req;
}

std::string HttpHeader::buildHeader(int status_code, const std::string &desc, const std::string &ver, const std::string &req_method)
{
    processHeader(status_code, req_method);
    std::string rsp = (!ver.empty()?ver:VersionHTTP1_1) + " " + std::to_string(status_code);
    if (!desc.empty()) {
        rsp += " " + desc;
    }
    rsp += "\r\n";
    for (auto &kv : header_vec_) {
        rsp += kv.first + ": " + kv.second + "\r\n";
    }
    rsp += "\r\n";
    return rsp;
}

void HttpHeader::reset()
{
    has_content_length_ = false;
    content_length_ = 0;
    is_chunked_ = false;
    has_body_ = false;
    encoding_type_.clear();
    header_vec_.clear();
}

void HttpHeader::setHeaders(const HeaderVector &headers)
{
    for (auto &kv : headers) {
        addHeader(kv.first, kv.second);
    }
}

void HttpHeader::setHeaders(HeaderVector &&headers)
{
    setHeaders(headers);
}

HttpHeader& HttpHeader::operator= (const HttpHeader &other)
{
    if (&other != this) {
        is_chunked_ = other.is_chunked_;
        has_content_length_ = other.has_content_length_;
        content_length_ = other.content_length_;
        has_body_ = other.has_body_;
        encoding_type_ = other.encoding_type_;
        header_vec_ = other.header_vec_;
    }
    
    return *this;
}

HttpHeader& HttpHeader::operator= (HttpHeader &&other)
{
    if (&other != this) {
        is_chunked_ = other.is_chunked_;
        has_content_length_ = other.has_content_length_;
        content_length_ = other.content_length_;
        has_body_ = other.has_body_;
        encoding_type_ = std::move(other.encoding_type_);
        header_vec_ = std::move(other.header_vec_);
    }
    
    return *this;
}

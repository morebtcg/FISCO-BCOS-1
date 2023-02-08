/*
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file CompositeBuffer.h
 * @author: octopus
 * @date 2021-08-23
 */
#pragma once

#include "bcos-utilities/ObjectCounter.h"
#include <bcos-utilities/Common.h>
#include <boost/asio/buffer.hpp>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace bcos
{

class CompositeBuffer : bcos::ObjectCounter<CompositeBuffer>
{
public:
    using Ptr = std::shared_ptr<CompositeBuffer>;
    using ConstPtr = std::shared_ptr<const CompositeBuffer>;

    CompositeBuffer() = default;
    ~CompositeBuffer() = default;

    CompositeBuffer(const CompositeBuffer&) = delete;
    CompositeBuffer(CompositeBuffer&&) = delete;
    CompositeBuffer& operator=(const CompositeBuffer&) = delete;
    CompositeBuffer& operator=(CompositeBuffer&& compositeBuffer) noexcept
    {
        this->m_buffers = std::move(compositeBuffer.m_buffers);
        compositeBuffer.m_buffers.clear();
        return *this;
    }
    //
    CompositeBuffer(bcos::bytes& _buffer) { m_buffers.push_back(std::move(_buffer)); }
    CompositeBuffer(bcos::bytes&& _buffer) { m_buffers.push_back(std::move(_buffer)); }

    void append(bcos::bytes& _buffer, bool appendToHead = false)
    {
        if (appendToHead)
        {
            m_buffers.insert(m_buffers.begin(), std::move(_buffer));
        }
        else
        {
            m_buffers.push_back(std::move(_buffer));
        }
    }

    void append(std::vector<bcos::bytes>& _buffer, bool appendToHead = false)
    {
        if (appendToHead)
        {
            m_buffers.insert(m_buffers.begin(), _buffer.begin(), _buffer.end());
        }
        else
        {
            m_buffers.insert(m_buffers.end(), _buffer.begin(), _buffer.end());
        }
    }

    bcos::bytes toSingleBuffer()
    {
        if (m_buffers.empty())
        {
            return bcos::bytes{};
        }

        bcos::bytes retBuffer = std::move(m_buffers.front());

        std::size_t totalSize = retBuffer.size();
        std::for_each(m_buffers.begin() + 1, m_buffers.end(),
            [&totalSize](const bcos::bytes& _buffer) { totalSize += _buffer.size(); });
        retBuffer.reserve(totalSize);

        std::for_each(m_buffers.begin() + 1, m_buffers.end(), [&retBuffer](bcos::bytes& _buffer) {
            retBuffer.insert(retBuffer.end(), _buffer.begin(), _buffer.end());
        });

        m_buffers.clear();
        return retBuffer;
    }

    // Notice:
    bcos::bytes toSingleBuffer() const
    {
        bcos::bytes retBuffer;
        std::size_t totalSize = 0;
        std::for_each(m_buffers.begin(), m_buffers.end(),
            [&totalSize](const bcos::bytes& _buffer) { totalSize += _buffer.size(); });
        retBuffer.reserve(totalSize);

        std::for_each(m_buffers.begin(), m_buffers.end(), [&retBuffer](const bcos::bytes& _buffer) {
            retBuffer.insert(retBuffer.end(), _buffer.begin(), _buffer.end());
        });

        return retBuffer;
    }

    std::size_t payloadSize() const
    {
        std::size_t totalSize = 0;

        std::for_each(m_buffers.begin(), m_buffers.end(),
            [&totalSize](const bcos::bytes& _buffer) { totalSize += _buffer.size(); });

        return totalSize;
    }

    std::size_t length() const { return m_buffers.size(); }
    void reset() { m_buffers.clear(); }

    const std::vector<bcos::bytes>& buffers() const { return m_buffers; }
    std::vector<bcos::bytes>& buffers() { return m_buffers; }

    std::vector<boost::asio::const_buffer> toMultiBuffers() const
    {
        std::vector<boost::asio::const_buffer> bufs;
        bufs.reserve(m_buffers.size());
        std::for_each(m_buffers.begin(), m_buffers.end(),
            [&](const bcos::bytes& _buffer) { bufs.push_back(boost::asio::buffer(_buffer)); });

        return bufs;
    }

private:
    std::vector<bcos::bytes> m_buffers;
};

class CompositeBufferFactory
{
public:
    static CompositeBuffer::Ptr build() { return std::make_shared<CompositeBuffer>(); }

    static CompositeBuffer::Ptr build(bcos::bytes&& _buffer)
    {
        return std::make_shared<CompositeBuffer>(std::move(_buffer));
    }

    static CompositeBuffer::Ptr build(bcos::bytes& _buffer)
    {
        return std::make_shared<CompositeBuffer>(std::move(_buffer));
    }

    // TODO: For temporary existence , just for compile
    static CompositeBuffer::Ptr build(bytesConstRef _payload)
    {
        return std::make_shared<CompositeBuffer>(bcos::bytes{_payload.begin(), _payload.end()});
    }
};

}  // namespace bcos
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
 * @brief: threadpool that can execute tasks asyncly
 *
 * @file ThreadPool.h
 * @author: yujiechen
 * @date 2021-02-26
 */

#pragma once
#include "Common.h"
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/thread/thread.hpp>
#include <iosfwd>
#include <memory>

namespace bcos
{
class ThreadPool
{
public:
    using Ptr = std::shared_ptr<ThreadPool>;

    ThreadPool(const std::string& threadName, size_t size)
    {
        for (auto i = 0U; i < size; ++i)
        {
            m_workers.create_thread([this, threadName = threadName] {
                bcos::pthread_setThreadName(threadName);
                boost::asio::io_service::work work(m_ioService);
                m_ioService.run();
            });
        }
    }
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) noexcept = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) noexcept = delete;
    ~ThreadPool() noexcept { stop(); }

    void stop() noexcept
    {
        if (!m_ioService.stopped())
        {
            try
            {
                m_ioService.stop();

                if (!m_workers.is_this_thread_in())
                {
                    m_workers.join_all();
                }
            }
            catch (std::exception& e)
            {
                std::cout << "Error on stop threadpool!" << boost::diagnostic_information(e)
                          << std::endl;
            }
            catch (...)
            {
                std::cout << "Unknown exception!" << std::endl;
            }
        }
    }

    template <class F>
    void enqueue(F task)
    {
        m_ioService.post(std::move(task));
    }

    bool hasStopped() { return m_ioService.stopped(); }

private:
    boost::thread_group m_workers;
    boost::asio::io_service m_ioService;
};

}  // namespace bcos

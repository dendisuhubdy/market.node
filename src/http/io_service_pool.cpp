//
// io_service_pool.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2013 Alex Caudill (alex at furosys.com)
//
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "http/server.hpp"
#include <stdexcept>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

namespace http {
    namespace market {
        io_service_pool::io_service_pool(std::size_t pool_size) : next_io_service_(0) {
            if (pool_size == 0) {
                throw std::runtime_error("io_service_pool size is 0");
            }

            // Give all the io_services work to do so that their run() functions will not
            // exit until they are explicitly stopped.
            for (std::size_t i = 0; i < pool_size; ++i) {
                io_service_ptr io_service(new boost::asio::io_service);
                work_ptr work(new boost::asio::io_service::work(*io_service));
                io_services_.push_back(io_service);
                 work_.push_back(work);
            }
        }

        void io_service_pool::run() {
            // Create a pool of threads to run all of the io_services.
            std::vector<boost::shared_ptr<boost::thread> > threads;
            for (std::size_t i = 0; i < io_services_.size(); ++i) {
                boost::shared_ptr<boost::thread> thread(
                    new boost::thread(
                        boost::bind(
                            &boost::asio::io_service::run, 
                            io_services_[i]
                        )
                    )
                );
                threads.push_back(thread);
            }

            // Wait for all threads in the pool to exit.
            for (std::size_t i = 0; i < threads.size(); ++i) { 
                threads[i]->join();
            }
        }

        void io_service_pool::stop() {
            // Explicitly stop all io_services.
            for (std::size_t i = 0; i < io_services_.size(); ++i) { 
                io_services_[i]->stop();
            }
        }

        boost::asio::io_service& io_service_pool::get_io_service() {
            // Use a round-robin scheme to choose the next io_service to use.
            boost::asio::io_service& io_service = *io_services_[next_io_service_];
            ++next_io_service_;
            if (next_io_service_ == io_services_.size()) { 
                next_io_service_ = 0;
            }
            return io_service;
        }
    } // namespace market
} // namespace http
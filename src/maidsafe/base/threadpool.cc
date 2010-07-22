/* Copyright (c) 2009 maidsafe.net limited
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    * Neither the name of the maidsafe.net limited nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "maidsafe/base/threadpool.h"

namespace base {

ThreadedCallContainer::ThreadedCallContainer(const size_t &thread_count)
    : running_(false), mutex_(), condition_(), threads_(), functors_() {
  for (size_t i = 0; i < thread_count; ++i) {
    threads_.push_back(boost::shared_ptr<boost::thread>(new boost::thread(
        &ThreadedCallContainer::Run, this)));
  }
}

ThreadedCallContainer::~ThreadedCallContainer() {
  {
    boost::mutex::scoped_lock lock(mutex_);
    running_ = false;
    condition_.notify_all();
  }
  for (size_t i = 0; i < threads_.size(); ++i) {
    threads_.at(i)->join();
  }
}

void ThreadedCallContainer::Enqueue(VoidFunctor functor) {
  boost::mutex::scoped_lock lock(mutex_);
  if (!running_)
    return;
  functors_.push(functor);
  condition_.notify_all();
}

void ThreadedCallContainer::Wait() {
  boost::mutex::scoped_lock lock(mutex_);
  while (running_ && !functors_.empty())
    condition_.wait(lock);
}

void ThreadedCallContainer::Run() {
  boost::mutex::scoped_lock lock(mutex_);
  running_ = true;
  while (running_) {
    while (running_ && functors_.empty()) {
      condition_.wait(lock);
    }
    while (!functors_.empty()) {
      // grab the first functor from the queue, but allow other threads to
      // operate while executing it
      VoidFunctor f = functors_.front();
      mutex_.unlock();
      f();
      mutex_.lock();
      functors_.pop();
      condition_.notify_all();
    }
  }
}

}  // namespace base
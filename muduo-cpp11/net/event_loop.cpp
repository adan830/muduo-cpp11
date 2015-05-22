// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)

#include "muduo-cpp11/net/event_loop.h"

#include <assert.h>
#include <signal.h>
#include <sys/eventfd.h>

#include <functional>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/net/channel.h"
#include "muduo-cpp11/net/poller.h"
#include "muduo-cpp11/net/sockets_ops.h"
#include "muduo-cpp11/net/timer_queue.h"

namespace muduo_cpp11 {
namespace net {

namespace {

__thread EventLoop* t_loop_in_this_thread = 0;

const int kPollTimeMs = 10000;

int CreateEventfd() {
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    LOG(ERROR) << "Failed in eventfd";
    abort();
  }
  return evtfd;
}

class IgnoreSigPipe {
 public:
  IgnoreSigPipe() {
    ::signal(SIGPIPE, SIG_IGN);
  }
};

IgnoreSigPipe init_obj;

}  // namespace

EventLoop* EventLoop::GetEventLoopOfCurrentThread() {
  return t_loop_in_this_thread;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      event_handling_(false),
      calling_pending_functors_(false),
      iteration_(0),
      thread_id_(gettid()),
      poller_(Poller::NewDefaultPoller(this)),
      timer_queue_(new TimerQueue(this)),
      wakeup_fd_(CreateEventfd()),
      wakeup_channel_(new Channel(this, wakeup_fd_)),
      current_active_channel_(NULL) {
  DLOG(INFO) << "EventLoop created " << this << " in thread " << thread_id_;

  if (t_loop_in_this_thread) {
    LOG(FATAL) << "Another EventLoop " << t_loop_in_this_thread
               << " exists in this thread " << thread_id_;
  } else {
    t_loop_in_this_thread = this;
  }

  wakeup_channel_->set_read_callback(std::bind(&EventLoop::HandleRead, this));
  wakeup_channel_->EnableReading();  // we are always reading the wakeupfd
}

EventLoop::~EventLoop() {
  DLOG(INFO) << "EventLoop " << this << " of thread " << thread_id_
             << " destructs in thread " << gettid();

  wakeup_channel_->DisableAll();
  wakeup_channel_->Remove();
  ::close(wakeup_fd_);
  t_loop_in_this_thread = NULL;
}

void EventLoop::Loop() {
  assert(!looping_);
  AssertInLoopThread();

  looping_ = true;
  quit_ = false;  // FIXME: what if someone calls Quit() before Loop() ?

  VLOG(1) << "EventLoop " << this << " start looping";

  while (!quit_) {
    active_channels_.clear();
    poll_return_time_ = poller_->Poll(kPollTimeMs, &active_channels_);
    ++iteration_;

    if (VLOG_IS_ON(1)) {
      PrintActiveChannels();
    }

    // TODO sort channel by priority
    event_handling_ = true;
    for (ChannelList::iterator it = active_channels_.begin();
         it != active_channels_.end();
         ++it) {
      current_active_channel_ = *it;
      current_active_channel_->HandleEvent(poll_return_time_);
    }

    current_active_channel_ = NULL;
    event_handling_ = false;

    DoPendingFunctors();
  }

  VLOG(1) << "EventLoop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::Quit() {
  // FIXME: Make sure Quit() when polling.
  quit_ = true;

  // There is a chance that Loop() just executes while(!quit_) and exists,
  // then EventLoop destructs, then we are accessing an invalid object.
  // Can be fixed using mutex_ in both places.
  if (!IsInLoopThread()) {
    Wakeup();
  }
}

void EventLoop::RunInLoop(const Functor& cb) {
  if (IsInLoopThread()) {
    cb();
  } else {
    QueueInLoop(cb);
  }
}

void EventLoop::QueueInLoop(const Functor& cb) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_functors_.push_back(cb);
  }

  if (!IsInLoopThread() || calling_pending_functors_) {
    Wakeup();
  }
}

TimerId EventLoop::RunAt(const Timestamp& time, const TimerCallback& cb) {
  return timer_queue_->AddTimer(cb, time, 0.0);
}

TimerId EventLoop::RunAfter(double delay, const TimerCallback& cb) {
  Timestamp time(AddTime(Timestamp::Now(), delay));
  return RunAt(time, cb);
}

TimerId EventLoop::RunEvery(double interval, const TimerCallback& cb) {
  Timestamp time(AddTime(Timestamp::Now(), interval));
  return timer_queue_->AddTimer(cb, time, interval);
}

void EventLoop::Cancel(TimerId timerId) {
  return timer_queue_->Cancel(timerId);
}

void EventLoop::UpdateChannel(Channel* channel) {
  assert(channel->owner_loop() == this);
  AssertInLoopThread();
  poller_->UpdateChannel(channel);
}

void EventLoop::RemoveChannel(Channel* channel) {
  assert(channel->owner_loop() == this);
  AssertInLoopThread();

  if (event_handling_) {
    assert(current_active_channel_ == channel ||
           std::find(active_channels_.begin(),
                     active_channels_.end(),
                     channel) == active_channels_.end());
  }

  poller_->RemoveChannel(channel);
}

bool EventLoop::HasChannel(Channel* channel) {
  assert(channel->owner_loop() == this);
  AssertInLoopThread();
  return poller_->HasChannel(channel);
}

bool EventLoop::IsInLoopThread() const {
  return thread_id_ == gettid();
}

void EventLoop::AbortNotInLoopThread() {
  LOG(FATAL) << "EventLoop::AbortNotInLoopThread - EventLoop " << this
             << " was created in thread_id_ = " << thread_id_
             << ", current thread id = " << gettid();
}

void EventLoop::Wakeup() {
  uint64_t one = 1;
  ssize_t n = sockets::Write(wakeup_fd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG(ERROR) << "EventLoop::Wakeup() writes " << n << " bytes instead of 8";
  }
}

void EventLoop::HandleRead() {
  uint64_t one = 1;
  ssize_t n = sockets::Read(wakeup_fd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG(ERROR) << "EventLoop::HandleRead() reads " << n << " bytes instead of 8";
  }
}

void EventLoop::DoPendingFunctors() {
  std::vector<Functor> functors;
  calling_pending_functors_ = true;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    functors.swap(pending_functors_);
  }

  for (size_t i = 0; i < functors.size(); ++i) {
    functors[i]();
  }

  calling_pending_functors_ = false;
}

void EventLoop::PrintActiveChannels() const {
  for (ChannelList::const_iterator it = active_channels_.begin();
       it != active_channels_.end();
       ++it) {
    const Channel* ch = *it;
    VLOG(1) << "{" << ch->REventsToString() << "} ";
  }
}

}  // namespace net
}  // namespace muduo_cpp11

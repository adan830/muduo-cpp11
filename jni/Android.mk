# Yifan Fan (yifan.fan.1983@gmail.com)

LOCAL_PATH := $(call my-dir)

BASE_SRC_FILES := \
    muduo-cpp11/base/logging.cpp 		\
    muduo-cpp11/base/thread_pool.cpp 		\
    muduo-cpp11/base/timestamp.cpp

NET_SRC_FILES := \
    muduo-cpp11/net/acceptor.cpp 		\
    muduo-cpp11/net/buffer.cpp 			\
    muduo-cpp11/net/channel.cpp 		\
    muduo-cpp11/net/connector.cpp 		\
    muduo-cpp11/net/event_loop.cpp 		\
    muduo-cpp11/net/event_loop_thread.cpp 	\
    muduo-cpp11/net/event_loop_thread_pool.cpp 	\
    muduo-cpp11/net/inet_address.cpp 		\
    muduo-cpp11/net/poller.cpp 			\
    muduo-cpp11/net/socket.cpp 			\
    muduo-cpp11/net/sockets_ops.cpp 		\
    muduo-cpp11/net/tcp_client.cpp 		\
    muduo-cpp11/net/tcp_connection.cpp 		\
    muduo-cpp11/net/tcp_server.cpp 		\
    muduo-cpp11/net/timer.cpp 			\
    muduo-cpp11/net/timer_queue.cpp 		\
    muduo-cpp11/net/poller/default_poller.cpp   \
    muduo-cpp11/net/poller/poll_poller.cpp

# =======================================================
include $(CLEAR_VARS)

LOCAL_MODULE := muduo_cpp11
LOCAL_SRC_FILES := $(BASE_SRC_FILES) $(NET_SRC_FILES)
LOCAL_LDFLAGS := -llog

LOCAL_CPP_EXTENSION := .cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/android 			\
    $(LOCAL_PATH)/base 				\
    $(LOCAL_PATH)/net

include $(BUILD_SHARED_LIBRARY)

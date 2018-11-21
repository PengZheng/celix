/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#include <iostream>
#include <mutex>
#include <memory.h>
#include <vector>
#include <string>
#include <sstream>

#include <stdlib.h>
#include <assert.h>

#include <sys/epoll.h>
#include <arpa/inet.h>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>

#include <pubsub_serializer.h>
#include <pubsub/subscriber.h>
#include <pubsub_constants.h>
#include <pubsub_endpoint.h>
#include <log_helper.h>

#include "pubsub_nanomsg_topic_receiver.h"
#include "pubsub_psa_nanomsg_constants.h"
#include "pubsub_nanomsg_common.h"
#include "pubsub_topology_manager.h"

//TODO see if block and wakeup (reset) also works
#define PSA_NANOMSG_RECV_TIMEOUT 1000

/*
#define L_DEBUG(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)
*/
#define L_DEBUG printf
#define L_INFO printf
#define L_WARN printf
#define L_ERROR printf





//static void pubsub_nanoMsgTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props,
//                                                         const celix_bundle_t *owner);


pubsub::nanomsg::topic_receiver::topic_receiver(celix_bundle_context_t *_ctx,
        log_helper_t *_logHelper,
        const char *_scope,
        const char *_topic,
        long _serializerSvcId,
        pubsub_serializer_service_t *_serializer) : m_serializerSvcId{_serializerSvcId}, m_scope{_scope}, m_topic{_topic} {
    ctx = _ctx;
    logHelper = _logHelper;
    serializer = _serializer;
    psa_nanomsg_setScopeAndTopicFilter(m_scope, m_topic, m_scopeAndTopicFilter);

    m_nanoMsgSocket = nn_socket(AF_SP, NN_BUS);
    if (m_nanoMsgSocket < 0) {
        // TODO throw error or something
        //free(receiver);
        //receiver = NULL;
        L_ERROR("[PSA_NANOMSG] Cannot create TopicReceiver for %s/%s", m_scope, m_topic);
    } else {
        int timeout = PSA_NANOMSG_RECV_TIMEOUT;
        if (nn_setsockopt(m_nanoMsgSocket , NN_SOL_SOCKET, NN_RCVTIMEO, &timeout,
                          sizeof (timeout)) < 0) {
            // TODO throw error or something
            //free(receiver);
            //receiver = NULL;
            L_ERROR("[PSA_NANOMSG] Cannot create TopicReceiver for %s/%s, set sockopt RECV_TIMEO failed", m_scope, m_topic);
        }

        char subscribeFilter[5];
        psa_nanomsg_setScopeAndTopicFilter(m_scope, m_topic, subscribeFilter);

        m_scope = strndup(m_scope, 1024 * 1024);
        m_topic = strndup(m_topic, 1024 * 1024);

        subscribers.map = hashMap_create(NULL, NULL, NULL, NULL);
        std::cout << "#### Creating subscirbers.map!! " << subscribers.map << "\n";
        //requestedConnections.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

        int size = snprintf(NULL, 0, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, m_topic);
        char buf[size + 1];
        snprintf(buf, (size_t) size + 1, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, m_topic);
        celix_service_tracking_options_t opts{};
        opts.filter.ignoreServiceLanguage = true;
        opts.filter.serviceName = PUBSUB_SUBSCRIBER_SERVICE_NAME;
        opts.filter.filter = buf;
        opts.callbackHandle = this;
        opts.addWithOwner = [](void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner) {
            static_cast<topic_receiver*>(handle)->addSubscriber(svc, props, svcOwner);
        };
        opts.removeWithOwner = [](void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner) {
            static_cast<topic_receiver*>(handle)->removeSubscriber(svc, props, svcOwner);
        };

        subscriberTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
        recvThread.running = true;

        recvThread.thread = std::thread([this]() {this->recvThread_exec();});
    }
}

pubsub::nanomsg::topic_receiver::~topic_receiver() {

        {
            std::lock_guard<std::mutex> _lock(recvThread.mutex);
            recvThread.running = false;
        }
        recvThread.thread.join();

        celix_bundleContext_stopTracker(ctx, subscriberTrackerId);

        hash_map_iterator_t iter=hash_map_iterator_t();
        {
            std::lock_guard<std::mutex> _lock(subscribers.mutex);
            iter = hashMapIterator_construct(subscribers.map);
            while (hashMapIterator_hasNext(&iter)) {
                psa_nanomsg_subscriber_entry_t *entry = static_cast<psa_nanomsg_subscriber_entry_t*>(hashMapIterator_nextValue(&iter));
                if (entry != NULL)  {
                    serializer->destroySerializerMap(serializer->handle, entry->msgTypes);
                    free(entry);
                }
            }
            hashMap_destroy(subscribers.map, false, false);
        }


//        {
//            std::lock_guard<std::mutex> _lock(requestedConnections.mutex);
//            iter = hashMapIterator_construct(requestedConnections.map);
//            while (hashMapIterator_hasNext(&iter)) {
//                psa_nanomsg_requested_connection_entry_t *entry = static_cast<psa_nanomsg_requested_connection_entry_t*>(hashMapIterator_nextValue(&iter));
//                if (entry != NULL) {
//                    free(entry->url);
//                    free(entry);
//                }
//            }
//            hashMap_destroy(requestedConnections.map, false, false);
//        }

        //celixThreadMutex_destroy(&receiver->subscribers.mutex);
        //celixThreadMutex_destroy(&receiver->requestedConnections.mutex);
        //celixThreadMutex_destroy(&receiver->recvThread.mutex);

        nn_close(m_nanoMsgSocket);

        free((void*)m_scope);
        free((void*)m_topic);
}

const char* pubsub::nanomsg::topic_receiver::scope() const {
    return m_scope;
}
const char* pubsub::nanomsg::topic_receiver::topic() const {
    return m_topic;
}

long pubsub::nanomsg::topic_receiver::serializerSvcId() const {
    return m_serializerSvcId;
}

void pubsub::nanomsg::topic_receiver::listConnections(std::vector<std::string> &connectedUrls,
                                                 std::vector<std::string> &unconnectedUrls) {
    std::lock_guard<std::mutex> _lock(requestedConnections.mutex);
    for (auto entry : requestedConnections.map) {
        if (entry.second.isConnected()) {
            connectedUrls.push_back(std::string(entry.second.getUrl()));
        } else {
            unconnectedUrls.push_back(std::string(entry.second.getUrl()));
        }
    }
}


void pubsub::nanomsg::topic_receiver::connectTo(const char *url) {
    L_DEBUG("[PSA_ZMQ] TopicReceiver %s/%s connecting to zmq url %s", m_scope, m_topic, url);

    std::lock_guard<std::mutex> _lock(requestedConnections.mutex);
    auto entry  = requestedConnections.map.find(url);
    if (entry == requestedConnections.map.end()) {
        requestedConnections.map.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(std::string(url)),
                std::forward_as_tuple(url, -1));
    }
    if (!entry->second.isConnected()) {
        int connection_id = nn_connect(m_nanoMsgSocket, url);
        if (connection_id >= 0) {
            entry->second.setConnected(true);
            entry->second.setId(connection_id);
        } else {
            L_WARN("[PSA_NANOMSG] Error connecting to NANOMSG url %s. (%s)", url, strerror(errno));
        }
    }
}

void pubsub::nanomsg::topic_receiver::disconnectFrom(const char *url) {
    L_DEBUG("[PSA ZMQ] TopicReceiver %s/%s disconnect from zmq url %s", m_scope, m_topic, url);

    std::lock_guard<std::mutex> _lock(requestedConnections.mutex);
    auto entry = requestedConnections.map.find(url);
    if (entry != requestedConnections.map.end()) {
        if (entry->second.isConnected()) {
            if (nn_shutdown(m_nanoMsgSocket, entry->second.getId()) == 0) {
                entry->second.setConnected(false);
            } else {
                L_WARN("[PSA_NANOMSG] Error disconnecting from nanomsg url %s, id %d. (%s)", url, entry->second.getId(),
                       strerror(errno));
            }
        }
        requestedConnections.map.erase(url);
        std::cerr << "REMOVING connection " << url << std::endl;
    } else {
        std::cerr << "Disconnecting from unknown URL " << url << std::endl;
    }
}

void pubsub::nanomsg::topic_receiver::addSubscriber(void *svc, const celix_properties_t *props,
                                                      const celix_bundle_t *bnd) {
    long bndId = celix_bundle_getId(bnd);
    const char *subScope = celix_properties_get(props, PUBSUB_SUBSCRIBER_SCOPE, "default");
    if (strncmp(subScope, m_scope, strlen(m_scope)) != 0) {
        //not the same scope. ignore
        return;
    }

    std::lock_guard<std::mutex> _lock(subscribers.mutex);
    psa_nanomsg_subscriber_entry_t *entry = static_cast<psa_nanomsg_subscriber_entry_t*>(hashMap_get(subscribers.map, (void*)bndId));
    if (entry != NULL) {
        entry->usageCount += 1;
    } else {
        //new create entry
        entry = static_cast<psa_nanomsg_subscriber_entry_t*>(calloc(1, sizeof(*entry)));
        entry->usageCount = 1;
        entry->svc = static_cast<pubsub_subscriber_t*>(svc);

        int rc = serializer->createSerializerMap(serializer->handle, (celix_bundle_t*)bnd, &entry->msgTypes);
        if (rc == 0) {
            hashMap_put(subscribers.map, (void*)bndId, entry);
        } else {
            L_ERROR("[PSA_NANOMSG] Cannot create msg serializer map for TopicReceiver %s/%s", m_scope, m_topic);
            free(entry);
        }
    }
}

void pubsub::nanomsg::topic_receiver::removeSubscriber(void */*svc*/,
                                                         const celix_properties_t */*props*/, const celix_bundle_t *bnd) {
    long bndId = celix_bundle_getId(bnd);

    std::lock_guard<std::mutex> _lock(subscribers.mutex);
    psa_nanomsg_subscriber_entry_t *entry = static_cast<psa_nanomsg_subscriber_entry_t*>(hashMap_get(subscribers.map, (void*)bndId));
    if (entry != NULL) {
        entry->usageCount -= 1;
    }
    if (entry != NULL && entry->usageCount <= 0) {
        //remove entry
        hashMap_remove(subscribers.map, (void*)bndId);
        int rc = serializer->destroySerializerMap(serializer->handle, entry->msgTypes);
        if (rc != 0) {
            L_ERROR("[PSA_NANOMSG] Cannot destroy msg serializers map for TopicReceiver %s/%s", m_scope, m_topic);
        }
        free(entry);
    }
}

void pubsub::nanomsg::topic_receiver::processMsgForSubscriberEntry(psa_nanomsg_subscriber_entry_t* entry, const pubsub_nanmosg_msg_header_t *hdr, const char* payload, size_t payloadSize) {
    pubsub_msg_serializer_t* msgSer = static_cast<pubsub_msg_serializer_t*>(hashMap_get(entry->msgTypes, (void*)(uintptr_t)(hdr->type)));
    pubsub_subscriber_t *svc = entry->svc;

    if (msgSer!= NULL) {
        void *deserializedMsg = NULL;
        bool validVersion = psa_nanomsg_checkVersion(msgSer->msgVersion, hdr);
        if (validVersion) {
            celix_status_t status = msgSer->deserialize(msgSer, payload, payloadSize, &deserializedMsg);
            if(status == CELIX_SUCCESS) {
                bool release = false;
                svc->receive(svc->handle, msgSer->msgName, msgSer->msgId, deserializedMsg, NULL, &release);
                if (release) {
                    msgSer->freeMsg(msgSer->handle, deserializedMsg);
                }
            } else {
                //L_WARN("[PSA_NANOMSG_TR] Cannot deserialize msg type %s for scope/topic %s/%s", msgSer->msgName, scope, topic);
            }
        }
    } else {
        L_WARN("[PSA_NANOMSG_TR] Cannot find serializer for type id %i", hdr->type);
    }
}

void pubsub::nanomsg::topic_receiver::processMsg(const pubsub_nanmosg_msg_header_t *hdr, const char *payload, size_t payloadSize) {
    std::lock_guard<std::mutex> _lock(subscribers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_nanomsg_subscriber_entry_t *entry = static_cast<psa_nanomsg_subscriber_entry_t*>(hashMapIterator_nextValue(&iter));
        if (entry != NULL) {
            processMsgForSubscriberEntry(entry, hdr, payload, payloadSize);
        }
    }
}

struct Message {
    pubsub_nanmosg_msg_header_t header;
    char payload[];
};

void pubsub::nanomsg::topic_receiver::recvThread_exec() {
    bool running{};
    {
        std::lock_guard<std::mutex> _lock(recvThread.mutex);
        running = recvThread.running;
    }
    while (running) {
        Message *msg = nullptr;
        nn_iovec iov[2];
        iov[0].iov_base = &msg;
        iov[0].iov_len = NN_MSG;

        nn_msghdr msgHdr;
        memset(&msgHdr, 0, sizeof(msgHdr));

        msgHdr.msg_iov = iov;
        msgHdr.msg_iovlen = 1;

        msgHdr.msg_control = nullptr;
        msgHdr.msg_controllen = 0;

        errno = 0;
        int recvBytes = nn_recvmsg(m_nanoMsgSocket, &msgHdr, 0);
        if (msg && static_cast<unsigned long>(recvBytes) >= sizeof(pubsub_nanmosg_msg_header_t)) {
            processMsg(&msg->header, msg->payload, recvBytes-sizeof(msg->header));
            nn_freemsg(msg);
        } else if (recvBytes >= 0) {
            L_ERROR("[PSA_ZMQ_TR] Error receiving nanmosg msg, size (%d) smaller than header\n", recvBytes);
        } else if (errno == EAGAIN || errno == ETIMEDOUT) {
            //nop
        } else if (errno == EINTR) {
            L_DEBUG("[PSA_ZMQ_TR] zmsg_recv interrupted");
        } else {
            L_WARN("[PSA_ZMQ_TR] Error receiving zmq message: errno %d: %s\n", errno, strerror(errno));
        }
    } // while

}

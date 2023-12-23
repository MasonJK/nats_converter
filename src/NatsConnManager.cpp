// Copyright 2015-2018 The NATS Authors
#include <iostream>
#include <cstring>

#include "nats_converter/NatsConnManager.h"

namespace adcm
{
namespace etc
{


bool parseUrls(const char* urls, char** outUrl, unsigned char* numOfServer)
{
    int     num       = 0;
    bool    parseSuccess    = true;
    char*    commaPos = NULL;
    char*    ptr      = NULL;
    ptr = strdup(urls);

    if(ptr == NULL) {
        return false;
    }

    do {
        if(num == MAX_SERVERS) {
            parseSuccess = false;
            break;
        }

        outUrl[num++] = ptr;
        commaPos = strchr(ptr, ',');

        if(commaPos != NULL) {
            ptr = const_cast<char*>(commaPos + 1);
            *(commaPos) = '\0';

        } else {
            ptr = NULL;
        }
    } while(ptr != NULL);

    if(parseSuccess == true) {
        *numOfServer = static_cast<unsigned char>(num);
    }

    return parseSuccess;
}

natsStatus NatsConnManager::NatsExecute()
{
    natsStatus s = NATS_OK;
        
    s = NatsCreateNetsOptions();

    if (s == NATS_OK) 
    {
        std::cout << "NATS CreateNetsOptions" << std::endl;
        s = NatsConnection();
    }

    if (s != NATS_OK) 
    {
        std::cout << "NATS CreateNetsOptions Fail" << std::endl;
    }

    if (s != NATS_OK)
    {
        std::cout << "NATS Connection Error: " << s << " - " << natsStatus_GetText(s) << std::endl;
        NatsPrintLastErrorStack();
        opts = nullptr;
        conn = nullptr;

    }

    return s;

}

natsStatus NatsConnManager::NatsCreateNetsOptions()
{
    natsStatus s = NATS_OK;
    char* serverUrls[MAX_SERVERS];
    unsigned char numOfServer;

    if(natsOptions_Create(&opts) != NATS_OK) {
        s = NATS_NO_MEMORY;
    }else{
        s = natsOptions_SetUserInfo(opts,SERVER_ID,SERVER_PW);

    }

    if(s == NATS_OK) {
        memset(serverUrls, 0, sizeof(serverUrls));

        if(parseUrls(mServerUrl, serverUrls, &numOfServer) != false) {
            //std::cout << "num of server : " << (int)numOfServer << std::endl;
            for(int i = 0 ; i < numOfServer ; i++) {
                std::cout  << "[" << i << "] : "<< serverUrls[i] << std::endl;
            }

            std::cout << "SetServer..." << std::endl;
            s = natsOptions_SetServers(opts, const_cast<const char**>(serverUrls), numOfServer);

        } else {
            s = NATS_ERR;
            std::cout << "serverURL Parsing fail..." << std::endl;
        }
    }

    //std::cout << "Listening asynchronously on " << subj << std::endl;

    if((s == NATS_OK) && ((certFile != NULL) || (keyFile != NULL))) {
        s = natsOptions_LoadCertificatesChain(opts, certFile, keyFile);
    }

    if(subscribeFlag != Mode::Default) {
        std::cout << "Not want subscribe..." << std::endl;
        std::cout << "skip natsOptions_SetErrorHandler()" << std::endl;
    } else if(asyncCb != NULL) {
        s = natsOptions_SetErrorHandler(opts, asyncCb, this);

    } else {
        s = NATS_ERR;
        std::cout << "asyncCb is null..." << std::endl;
    }

    if(s != NATS_OK) {
        std::cout << "Error parsing arguments : "<< s << " - " << natsStatus_GetText(s) << std::endl;
        nats_PrintLastErrorStack(stderr);
        natsOptions_Destroy(opts);
        nats_Close();
    }

    return s;
}

natsStatus NatsConnManager::NatsConnection()
{
    natsStatus s = NATS_OK;

    if(s == NATS_OK) {
        s = natsConnection_Connect(&conn, opts);
    }

    if(s == NATS_OK) {
        std::cout  << "Connection..." << std::endl;
        if(subscribeFlag == Mode::Publish_Only) {
            std::cout  << "Not want subscribe..." << std::endl;
            std::cout  << "skip natsConnection_Subscribe()" << std::endl;
        } else if(onMsgCb != NULL && subj.size() != 0) {
            std::cout  << "natsConnection_Subscribe()" << std::endl;

            //s = natsConnection_Subscribe(&sub, conn, subj, onMsgCb, NULL);

            for (const char* subject : subj)
            {
                s = natsConnection_Subscribe(&sub, conn, subject, onMsgCb, NULL);
                if (s != NATS_OK) 
                {
                    s = NATS_ERR;
                    std::cout << "Failed to subscribe to subject: " << subject << std::endl;
                    break;
                }                
            }

        } else {
            s = NATS_ERR;
            std::cout  << "onMsgCb or subj is null..." << std::endl;
        }

        // For maximum performance, set no limit on the number of pending messages.
        // if(s == NATS_OK && subscribeFlag == Mode::Publish_Only) {
        //     s = natsSubscription_SetPendingLimits(sub, -1, -1);
        // }
    }
    
    if(s == NATS_OK) {
        s = natsStatistics_Create(&stats);
    }

    if(s != NATS_OK) {
        std::cout  << "Error parsing arguments : "<< s << " - " << natsStatus_GetText(s) << std::endl;
        nats_PrintLastErrorStack(stderr);
        natsOptions_Destroy(opts);
        nats_Close();
    }

    return s;
}

natsStatus NatsConnManager::printStats(int mode, natsConnection* conn, natsSubscription* sub, natsStatistics* stats)
{
    natsStatus  s           = NATS_OK;
    uint64_t    inMsgs      = 0;
    uint64_t    inBytes     = 0;
    uint64_t    outMsgs     = 0;
    uint64_t    outBytes    = 0;
    uint64_t    reconnected = 0;
    int         pending     = 0;
    int64_t     delivered   = 0;
    int64_t     sdropped    = 0;
    s = natsConnection_GetStats(conn, stats);

    if(s == NATS_OK) {
        s = natsStatistics_GetCounts(stats, &inMsgs, &inBytes,
                                     &outMsgs, &outBytes, &reconnected);
    }

    if((s == NATS_OK) && (sub != NULL)) {
        s = natsSubscription_GetStats(sub, &pending, NULL, NULL, NULL,
                                      &delivered, &sdropped);

        // Since we use AutoUnsubscribe(), when the max has been reached,
        // the subscription is automatically closed, so this call would
        // return "Invalid Subscription". Ignore this error.
        if(s == NATS_INVALID_SUBSCRIPTION) {
            s = NATS_OK;
            pending = 0;
        }
    }

    if(s == NATS_OK) {
        if(mode & STATS_IN) {
            //std::cout  << "In Msgs: " << inMsgs << PRIu64 << " - " << "In Bytes: " << inBytes << PRIu64 << " " << std::endl;
        }

        if(mode & STATS_OUT) {
            //std::cout  << "Out Msgs: " << outMsgs << PRIu64 << " - " << "Out Bytes: " << outBytes << PRIu64 << " " << std::endl;
        }

        if(mode & STATS_COUNT) {
            //std::cout  << "Delivered: " << delivered << PRId64 << " - " << std::endl;
            //std::cout  << "Pending: " << pending << " - " << std::endl;
            //std::cout  << "Dropped: " << sdropped << PRId64 << " - " << std::endl;
        }

        //std::cout  << "Reconnected: " << reconnected << PRId64 << std::endl;
    }

    return s;
}

natsStatus NatsConnManager::NatsPublish(const char* subject, const char*  payload)
{
    natsStatus  s           = NATS_OK;
    int         dataLen     = 0;
    dataLen = static_cast<int>(strlen(payload));

    if(subject != NULL) {
        s = natsConnection_Publish(conn, subject, static_cast<const void*>(payload), dataLen);

    } else {
        s = NATS_ERR;
        std::cout << "subject is null..." << std::endl;
    }

    if(s == NATS_OK) {
        s = natsConnection_FlushTimeout(conn, 1000);
    }

    if(s == NATS_OK) {
        printStats(STATS_OUT, conn, NULL, stats);

    } else {
        std::cout << "Error: " << s << " - " << natsStatus_GetText(s) << std::endl;
        nats_PrintLastErrorStack(stderr);
    }

    return s;
}

natsStatus NatsConnManager::NatsPublishJson(const char* subject)
{
    natsStatus  s           = NATS_OK;
    int         dataLen     = 0;
    std::stringstream payload;

    jsonObj.stringify(payload);
    dataLen = static_cast<int>(payload.str().size());
    
    //std::cout << " [KBJ] Publish data : " << payload.str().c_str() <<  ", length : " << dataLen << std::endl;

    if(subject != NULL) {
        s = natsConnection_Publish(conn, subject, static_cast<const void*>(payload.str().c_str()), dataLen);

    } else {
        s = NATS_ERR;
        std::cout << "subject is null..." << std::endl;
    }

    if(s == NATS_OK) {
        s = natsConnection_FlushTimeout(conn, 1000);
    }

    if(s == NATS_OK) {
        printStats(STATS_OUT, conn, NULL, stats);
        //printPerf("Sent");

    } else {
        std::cout << "Error: " << s << " - " << natsStatus_GetText(s) << std::endl;
        nats_PrintLastErrorStack(stderr);
    }

    return s;    
    
}

void NatsConnManager::ClearJsonData()
{
    jsonObj.clear();
}

void NatsConnManager::PrintSendData()
{
    std::stringstream sendData;

    jsonObj.stringify(sendData);
    std::cout << " Send data : " << sendData.str() <<  ", length : " << sendData.str().size() << std::endl;
}

void NatsConnManager::NatsPrintLastErrorStack()
{
    nats_PrintLastErrorStack(stderr);
}

void NatsConnManager::NatsSleep(int64_t sleepTime)
{
    nats_Sleep(sleepTime);
}

void NatsConnManager::NatsMsgDestroy(natsMsg* msg)
{
    natsMsg_Destroy(msg);
}

void NatsConnManager::NatsSubscriptionGetDropped(natsSubscription* sub, int64_t* msgs)
{
    natsSubscription_GetDropped(sub, msgs);
}

NatsConnManager::NatsConnManager(){
}

NatsConnManager::NatsConnManager(const char * serverUrl, const std::vector<const char*> subject, Mode mode)
{
    subj   = subject;
    mServerUrl = serverUrl;
    subscribeFlag = mode;
}

NatsConnManager::NatsConnManager(const char * serverUrl, const std::vector<const char*> subject, natsMsgHandler onMsgFunc, natsErrHandler asyncFunc, Mode mode)
{
    subj   = subject;
    mServerUrl = serverUrl;

    subscribeFlag = mode;
    onMsgCb = onMsgFunc;
    asyncCb = asyncFunc;
}

NatsConnManager::~NatsConnManager()
{
    // Destroy all our objects to avoid report of memory leak
    natsStatistics_Destroy(stats);
    natsSubscription_Destroy(sub);
    natsConnection_Destroy(conn);
    natsOptions_Destroy(opts);
    // To silence reports of memory still in used with valgrind
    nats_Close();
}


}
}

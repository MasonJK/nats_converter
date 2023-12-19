#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>

#include "nats_converter/NatsConnManager.h"

#define HMI_SERVER_URL  "https://nats.beyless.com"

std::shared_ptr<adcm::etc::NatsConnManager> natsManager;    
std::mutex mtx; // 뮤텍스 선언

void asyncCb(natsConnection* nc, natsSubscription* sub, natsStatus err, void* closure)
{
    std::cout << "Async error: " << err << " - " << natsStatus_GetText(err) << std::endl;
    natsManager->NatsSubscriptionGetDropped(sub, (int64_t*) &natsManager->dropped);
}

void onMsg(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* closure)
{
    const char* subject = NULL;
    
    // 뮤텍스를 사용하여 공유 변수 접근 보호
    std::lock_guard<std::mutex> lock(mtx);
    subject = natsMsg_GetSubject(msg);

    std::cout << "Received msg: [" << subject << " : " << natsMsg_GetDataLength(msg) << "]" << natsMsg_GetData(msg) << std::endl;
    // We should be using a mutex to protect those variables since
    // they are used from the subscription's delivery and the main
    // threads. For demo purposes, this is fine.
    
    // std::istringstream iss(subject);
    // std::vector<std::string> words;
    // std::string word;

    // while (std::getline(iss, word, '.')) 
    // {
    //     words.push_back(word);
    // }
    // std::cout << words[0] << " Data Category : " << words[1] << std::endl;

    natsManager->NatsMsgDestroy(msg);
}

int main(){
    
    natsStatus s = NATS_OK;
    //const char*  subject = "sensorData.*";
    std::vector<const char*> subject = {"test1.*", "test2.*"};
    natsManager = std::make_shared<adcm::etc::NatsConnManager>(HMI_SERVER_URL, subject, onMsg, asyncCb, adcm::etc::NatsConnManager::Mode::Default);
    s = natsManager->NatsExecute();

    for (int index = 0 ; index < 100 ; index ++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if(s == NATS_OK)
        {
            if(index % 2 == 0)
            {
                const char* pubSubject = "test1.JSON";
                std::cout << "NATS Publish : JON" << std::endl;
                natsManager->ClearJsonData();
                natsManager->addJsonData("a", 1234);
                natsManager->addJsonData("b", 'abcd');
                natsManager->addJsonData("c", 'abcd');
                natsManager->addJsonData("d", 1234);
                natsManager->addJsonData("e", 1234);
                natsManager->addJsonData("f", 1234);
                natsManager->addJsonData("g", 1234);
                natsManager->addJsonData("h", 1234);

                natsManager->PrintSendData();

                natsManager->NatsPublishJson(pubSubject);
            }
            else
            {
                const char* pubSubject = "test1.payload";
                const char* payload = "Hello World";
                std::cout << "NATS Publish : payload" << std::endl;

                natsManager->NatsPublish(pubSubject, payload);

            }

        }
        else
        {
            std::cout << "Nats Connection error" << std::endl;
            try{
            natsManager = std::make_shared<adcm::etc::NatsConnManager>(HMI_SERVER_URL,subject, onMsg, asyncCb, adcm::etc::NatsConnManager::Mode::Default);
            s = natsManager->NatsExecute();
            }catch(std::exception e)
            {
                std::cout << "Nats reConnection error" << std::endl;
            }
        }  

    }

}
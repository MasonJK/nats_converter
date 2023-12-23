#include <ros/ros.h>
#include <string>
#include <sensor_msgs/NavSatFix.h>
#include <iostream>
#include <mutex>

#include "nats_converter/NatsConnManager.h"

std::mutex mtx;

class RosNatsConverter{
public:
    RosNatsConverter(natsMsgHandler msgCb, natsErrHandler errCb, adcm::etc::NatsConnManager::Mode mode = adcm::etc::NatsConnManager::Mode::Default) : msgCb(msgCb), errCb(errCb), mode(mode){
        ros::NodeHandle nh;
        ros_topic_name = "/position_to_fix";
        nats_subjects = {"test1.*", "test2.*"};
        server_url = "https://nats.beyless.com";

        fix_subscriber = nh.subscribe(ros_topic_name, 10, &RosNatsConverter::fixCallback, this);

        initialize_nats_client();
    }

    void fixCallback(const sensor_msgs::NavSatFix::ConstPtr& msg){
        std::cout<<"NATS Publish : JSON"<<std::endl;
        
        natsManager.ClearJsonData();
        natsManager.addJsonData("latitude", msg->latitude);
        natsManager.addJsonData("longitude",msg->longitude);

        natsManager.PrintSendData();
        // natsManager.NatsPublishJson(ros_topic_name);
        natsManager.NatsPublishJson("test1.json");
    }

    void initialize_nats_client(){
        try{
            if(mode == adcm::etc::NatsConnManager::Mode::Default)
                natsManager = adcm::etc::NatsConnManager(server_url, nats_subjects, msgCb, errCb, mode);
            else
                natsManager = adcm::etc::NatsConnManager(server_url, nats_subjects, mode);
        }catch(...){
            ROS_INFO("Failed to initialize NATS client...");
        }
    }

    void executeManager(){
        status = natsManager.NatsExecute();
    }

    natsStatus getStatus(){ return status; }

private:
    const char* server_url;
    const char* ros_topic_name;
    std::vector<const char*> nats_subjects;

    natsStatus status;
    adcm::etc::NatsConnManager::Mode mode;
    
    natsMsgHandler msgCb;
    natsErrHandler errCb;

    ros::Subscriber fix_subscriber;
    adcm::etc::NatsConnManager natsManager;
};

void asyncCb(natsConnection* nc, natsSubscription* sub, natsStatus err, void* closure)
{
    adcm::etc::NatsConnManager* manager = static_cast<adcm::etc::NatsConnManager*>(closure);
    
    std::cout << "Async error: " << err << " - " << natsStatus_GetText(err) << std::endl;
    manager->NatsSubscriptionGetDropped(sub, (int64_t*) &manager->dropped);
    // natsSubscription_GetDropped(sub, msgs);
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

    natsMsg_Destroy(msg);
}

int main(int argc, char** argv){

    ros::init(argc, argv, "nats_converter");

    RosNatsConverter converter(onMsg, asyncCb);
    converter.executeManager();

    while(ros::ok()){
        if(converter.getStatus() == NATS_OK){
            ros::spinOnce();
        }else{
            std::cout<<"Nats Connection Error"<<std::endl;
            try{
                converter.initialize_nats_client();
                converter.executeManager();
            }catch(std::exception e){
                std::cout << "Nats Reconnection Error"<<std::endl;
            }
        }
    }
}
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
        ros_topic_name = "/transformed_fix";
        nats_subjects = {"sensorData.*", "test2.*"};
        server_url = "https://nats.beyless.com";

        fix_subscriber = nh.subscribe(ros_topic_name, 10, &RosNatsConverter::fixCallback, this);

        initialize_nats_client();
    }

    void fixCallback(const sensor_msgs::NavSatFix::ConstPtr& msg){
        // std::cout<<"NATS Publish : JSON"<<std::endl;
        
        natsManager.ClearJsonData();
        natsManager.addJsonData("long",msg->longitude);
        natsManager.addJsonData("lat", msg->latitude);

        natsManager.PrintSendData();
        // natsManager.NatsPublishJson(ros_topic_name);
        natsManager.NatsPublishJson("sensorData.json");
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
}

void onMsg(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* closure)
{
    const char* subject = NULL;

    std::lock_guard<std::mutex> lock(mtx);
    subject = natsMsg_GetSubject(msg);

    std::cout << "Received msg: [" << subject << " : " << natsMsg_GetDataLength(msg) << "]" << natsMsg_GetData(msg) << std::endl;

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
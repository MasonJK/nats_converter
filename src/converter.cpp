#include <ros/ros.h>
#include <string>
#include <sensor_msgs/NavSatFix.h>
#include <iostream>

#include "nats_converter/NatsConnManager.h"
#define HMI_SERVER_URL  "https://nats.beyless.com"

class RosNatsConverter{
public:
    RosNatsConverter(std::shared_ptr<adcm::etc::NatsConnManager> manager): manager(manager){
        ros::NodeHandle nh;
        topic_name = "/position_to_fix";
        fix_subscriber = nh.subscribe(topic_name, 10, &RosNatsConverter::fixCallback, this);
    }

    void fixCallback(const sensor_msgs::NavSatFix::ConstPtr& msg){
        std::cout<<"latidue : "<<msg->latitude<<", longitude : "<<msg->longitude<<std::endl;
        std::cout<<"NATS Publish : JSON"<<std::endl;
        
        manager->ClearJsonData();
        manager->addJsonData("latitude", msg->latitude);
        manager->addJsonData("longitude",msg->longitude);

        manager->PrintSendData();
        manager->NatsPublishJson(topic_name);
    }

private:
    std::shared_ptr<adcm::etc::NatsConnManager> manager;
    const char* topic_name;
    ros::Subscriber fix_subscriber;
};


int main(int argc, char** argv){
    ros::init(argc, argv, "nats_converter");

    std::vector<const char*> subjects;
    natsStatus status;

    std::shared_ptr<adcm::etc::NatsConnManager> natsManager;
    natsManager = std::make_shared<adcm::etc::NatsConnManager>(HMI_SERVER_URL, subjects, adcm::etc::NatsConnManager::Mode::Publish_Only);
    status = natsManager->NatsExecute();

    RosNatsConverter converter(natsManager);

    while(status == NATS_OK){
        std::cout<<"came in here"<<std::endl;
        if(status == NATS_OK){
            ros::spinOnce();
        }else{
            std::cout<<"Nats Connection Error"<<std::endl;
            try{
                natsManager = std::make_shared<adcm::etc::NatsConnManager>(HMI_SERVER_URL, subjects, adcm::etc::NatsConnManager::Mode::Publish_Only);
                status = natsManager->NatsExecute();
            }catch(std::exception e){
                std::cout << "Nats Reconnection Error"<<std::endl;
            }
        }
    }
}
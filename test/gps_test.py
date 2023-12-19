#!/usr/bin/env python3

import rospy
from sensor_msgs.msg import NavSatFix

def publish_navsat():
    # Initialize the ROS Node
    rospy.init_node('navsat_publisher', anonymous=True)

    # Create a Publisher
    pub = rospy.Publisher('/position_to_fix', NavSatFix, queue_size=10)

    # Set the rate of publishing
    rate = rospy.Rate(1) # 1 Hz

    while not rospy.is_shutdown():
        # Create a NavSatFix message
        navsat_msg = NavSatFix()

        # Fill in the data (example coordinates)
        navsat_msg.latitude = 40.7128 # Example latitude
        navsat_msg.longitude = -74.0060 # Example longitude
        navsat_msg.altitude = 10.0 # Example altitude

        # Publish the message
        pub.publish(navsat_msg)

        # Sleep to maintain the publishing rate
        rate.sleep()

if __name__ == '__main__':
    try:
        publish_navsat()
    except rospy.ROSInterruptException:
        pass

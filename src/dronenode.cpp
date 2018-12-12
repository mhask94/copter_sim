#include "dronenode.hpp"
#include "drone.hpp"
#include <ros/ros.h>
#include <string>
#include "nav_msgs/Odometry.h"
#include "rosflight_msgs/Command.h"
#include "3rd_party/quat.hpp"
#include <chrono>

namespace quad
{

DroneNode::DroneNode(int argc, char **argv) :
    m_argc{argc},
    m_argv{argv},
    m_use_ros{false},
    m_rate{m_drone.getDt()},
    m_ros_is_connected{false},
    m_is_running{false}
{
    m_inputs = m_drone.getEquilibriumInputs();
    m_states = m_drone.getStates();
    m_odom.pose.pose.position.x = 0;
    m_odom.pose.pose.position.y = 0;
    m_odom.pose.pose.position.z = 0;
    m_odom.pose.pose.orientation.w = 1;
    m_odom.pose.pose.orientation.x = 0;
    m_odom.pose.pose.orientation.y = 0;
    m_odom.pose.pose.orientation.z = 0;    
}

DroneNode::~DroneNode()
{
    if(ros::isStarted())
    {
        ros::shutdown();
        ros::waitForShutdown();
    }
    wait();
}

bool DroneNode::rosIsConnected() const
{
    return m_ros_is_connected;
}

bool DroneNode::init()
{
    ros::init(m_argc,m_argv,m_node_name);
    if (!ros::master::check())
        return false;
    return m_ros_is_connected = true;
}

bool DroneNode::init(const std::string &master_url, const std::string &host_url,bool use_ip)
{
    std::map<std::string,std::string> remappings;
    remappings["__master"] = master_url;
    if (use_ip)
        remappings["__ip"] = host_url;
    else
        remappings["__hostname"] = host_url;
    ros::init(remappings,m_node_name);
    if (!ros::master::check())
        return false;
    return m_ros_is_connected = true;
}

void DroneNode::setUseRos(const bool use_ros)
{
    m_use_ros = use_ros;
}

bool DroneNode::useRos() const
{
    return m_use_ros;
}

void DroneNode::run()
{
    m_is_running = true;
    if (m_use_ros)
        this->runRosNode();
    else
        this->runNode();
}

bool DroneNode::startNode()
{
    if (m_use_ros)
    {
        if (!ros::master::check())
            return false;
        this->setupRosComms();
    }
    start();
    return true;
}

void DroneNode::stopRunning()
{
    m_is_running = false;
}

void DroneNode::updateInputs(const dyn::uVec &inputs)
{
    m_inputs = inputs;
}

void DroneNode::runRosNode()
{
    ros::Rate publish_rate{500};
    while (ros::ok() && ros::master::check())
    {
        this->updateDynamics();
        m_state_pub.publish(m_odom);
        ros::spinOnce();
        publish_rate.sleep();
    }
    m_ros_is_connected = false;
    emit rosLostConnection();
}

void DroneNode::runNode()
{
    while (m_is_running)
    {
        auto t_start{std::chrono::high_resolution_clock::now()};
        this->updateDynamics();
        emit feedbackStates(m_states);
        emit statesChanged(&m_odom);
        while(std::chrono::duration<double,std::milli>(std::chrono::high_resolution_clock::now()-t_start).count() < m_rate) {}
    }
}

void DroneNode::setupRosComms(const std::string topic)
{
    ros::start();
    ros::NodeHandle nh;
    int states_queue_size{50};
    m_state_sub = nh.subscribe(topic, states_queue_size, &DroneNode::stateCallback, this);
    m_state_pub = nh.advertise<nav_msgs::Odometry>("states", states_queue_size);
}

void DroneNode::updateDynamics()
{
//    dyn::uVec u;
//    u << 0.55,0.56,0.55,0.56;
    m_drone.sendMotorCmds(m_inputs);
    m_states = m_drone.getStates();
    m_odom.pose.pose.position.x = m_states(dyn::PX);
    m_odom.pose.pose.position.y = m_states(dyn::PY);
    m_odom.pose.pose.position.z = m_states(dyn::PZ);
    quat::Quatd q{quat::Quatd::from_euler(m_states(dyn::RX),m_states(dyn::RY),m_states(dyn::RZ))};
    m_odom.pose.pose.orientation.w = q.w();
    m_odom.pose.pose.orientation.x = q.x();
    m_odom.pose.pose.orientation.y = q.y();
    m_odom.pose.pose.orientation.z = q.z();
}

void DroneNode::stateCallback(const nav_msgs::OdometryConstPtr &msg)
{
    m_odom.pose.pose.position.x = msg->pose.pose.position.x;
    m_odom.pose.pose.position.y = msg->pose.pose.position.y;
    m_odom.pose.pose.position.z = msg->pose.pose.position.z;
    m_odom.pose.pose.orientation.w = msg->pose.pose.orientation.w;
    m_odom.pose.pose.orientation.x = msg->pose.pose.orientation.x;
    m_odom.pose.pose.orientation.y = msg->pose.pose.orientation.y;
    m_odom.pose.pose.orientation.z = msg->pose.pose.orientation.z;

//    emit feedbackStates(&m_states);
    emit statesChanged(&m_odom);
}

} // end namespace quad

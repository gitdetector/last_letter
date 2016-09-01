#include <boost/bind.hpp>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/common/common.hh>
#include <stdio.h>

#include "gazebo_msgs/ModelState.h"
#include <gazebo/common/Plugin.hh>
#include <ros/ros.h>

//#include "math/Pose.hh"

namespace gazebo
{
  class modelStateBroadcaster : public ModelPlugin
  {
    public: void Load(physics::ModelPtr _parent, sdf::ElementPtr _sdf)
    {
      // Store the pointer to the model
      this->model = _parent;
      this->INS = this->model->GetLink("INS");

      if (!ros::isInitialized())
      {
        ROS_FATAL_STREAM("A ROS node for Gazebo has not been initialized, unable to load plugin. "
          << "Load the Gazebo system plugin 'libgazebo_ros_api_plugin.so' in the gazebo_ros package)");
        return;
      }
      // Listen to the update event. This event is broadcast every
      // simulation iteration.
      this->updateConnection = event::Events::ConnectWorldUpdateBegin(
          boost::bind(&modelStateBroadcaster::OnUpdate, this, _1));

      this->rosPub = this->rosHandle.advertise<gazebo_msgs::ModelState>("/" + this->model->GetName() + "/modelState",1); //model states publisher

      ROS_INFO("modelStateBroadcaster plugin initialized");
    }

    // Called by the world update start event
    public: void OnUpdate(const common::UpdateInfo & /*_info*/)
    {

      math::Vector3 tempVect;
      math::Quaternion tempQuat;
      math::Quaternion InertialQuat(0,0.7071,0.7071,0); // Rotation quaterion from NED to ENU
      math::Quaternion BodyQuat(0,1,0,0); // Rotation quaternion from default body to aerospace body frame

      // Read the model state
      this->modelPose = this->model->GetWorldPose();
      // Convert rotation
      this->modelPose.rot = this->modelPose.rot*BodyQuat;

        // Velocities required in the aerospace body frame
      this->modelVelLin = BodyQuat*this->model->GetRelativeLinearVel();
      this->modelVelAng = BodyQuat*this->model->GetRelativeAngularVel();

      this->modelState.model_name = this->model->GetName();

      tempVect = InertialQuat*this->modelPose.pos; // Rotate position vector to NED frame
      this->modelState.pose.position.x = tempVect.x;
      if (!std::isfinite(tempVect.x)) {ROS_ERROR("model_state_publisher.cpp plugin: NaN value in north"); return;}
      this->modelState.pose.position.y = tempVect.y;
      if (!std::isfinite(tempVect.y)) {ROS_ERROR("model_state_publisher.cpp plugin: NaN value in east"); return;}
      this->modelState.pose.position.z = tempVect.z;
      if (!std::isfinite(tempVect.z)) {ROS_ERROR("model_state_publisher.cpp plugin: NaN value in down"); return;}

      tempQuat = InertialQuat*this->modelPose.rot; // Rotate orientation quaternion to NED frame
      if (tempQuat.w<0)
      {
        tempQuat.w = -tempQuat.w;
        tempQuat.x = -tempQuat.x;
        tempQuat.y = -tempQuat.y;
        tempQuat.z = -tempQuat.z;
      }
      this->modelState.pose.orientation.x = tempQuat.x;
      this->modelState.pose.orientation.y = tempQuat.y;
      this->modelState.pose.orientation.z = tempQuat.z;
      this->modelState.pose.orientation.w = tempQuat.w;

      this->modelState.twist.linear.x = this->modelVelLin.x;
      if (!std::isfinite(this->modelVelLin.x)) {ROS_ERROR("model_state_publisher.cpp plugin: NaN value in u"); return;}
      this->modelState.twist.linear.y = this->modelVelLin.y;
      if (!std::isfinite(this->modelVelLin.y)) {ROS_ERROR("model_state_publisher.cpp plugin: NaN value in v"); return;}
      this->modelState.twist.linear.z = this->modelVelLin.z;
      if (!std::isfinite(this->modelVelLin.y)) {ROS_ERROR("model_state_publisher.cpp plugin: NaN value in w"); return;}

      this->modelState.twist.angular.x = this->modelVelAng.x;
      if (!std::isfinite(this->modelVelAng.x)) {ROS_ERROR("model_state_publisher.cpp plugin: NaN value in p"); return;}
      this->modelState.twist.angular.y = this->modelVelAng.y;
      if (!std::isfinite(this->modelVelAng.y)) {ROS_ERROR("model_state_publisher.cpp plugin: NaN value in q"); return;}
      this->modelState.twist.angular.z = this->modelVelAng.z;
      if (!std::isfinite(this->modelVelAng.z)) {ROS_ERROR("model_state_publisher.cpp plugin: NaN value in r"); return;}

      this->modelState.reference_frame = "world";

      this->rosPub.publish(this->modelState);
    }

    // Pointer to the model
    private: physics::ModelPtr model;
    // Pointer to the model INS link, which is in the NED frame
    private: physics::LinkPtr INS;

    // Pointer to the update event connection
    private: event::ConnectionPtr updateConnection;

    // ROS related variables
    private:
      ros::NodeHandle rosHandle;
      ros::Publisher rosPub;
      gazebo_msgs::ModelState modelState;
      math::Pose modelPose;
      math::Vector3 modelVelLin, modelVelAng;
  };

  // Register this plugin with the simulator
  GZ_REGISTER_MODEL_PLUGIN(modelStateBroadcaster)
}

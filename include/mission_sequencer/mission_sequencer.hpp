// Copyright (C) 2021 Martin Scheiber, Christoph Boehm,
// and others, Control of Networked Systems, University of Klagenfurt, Austria.
//
// All rights reserved.
//
// This software is licensed under the terms of the BSD-2-Clause-License with
// no commercial use allowed, the full terms of which are made available
// in the LICENSE file. No license in patents is granted.
//
// You can contact the authors at <martin.scheiber@ieee.org>,
// and <christoph.boehm@aau.at>

#ifndef MISSION_SEQUENCER_HPP
#define MISSION_SEQUENCER_HPP

#include <math.h>
#include <ros/ros.h>
#include <tf2/LinearMath/Matrix3x3.h>
#include <tf2/LinearMath/Quaternion.h>
// #include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <std_msgs/String.h>

// Include Subscriber Messages
#include <geometry_msgs/PoseStamped.h>
#include <mavros_msgs/ExtendedState.h>
#include <mavros_msgs/State.h>
#include <mission_sequencer/MissionRequest.h>

// Include Publisher Messages
#include <mission_sequencer/MissionResponse.h>
#include <mission_sequencer/MissionWaypointArray.h>

// Include Services
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/CommandLong.h>
#include <mavros_msgs/CommandTOL.h>
#include <mavros_msgs/SetMode.h>

// Waypoint list
#include "parse_waypoints.hpp"

#define RAD_TO_DEG (180.0 / M_PI)
#define DEG_TO_RAD (M_PI / 180.0)

namespace mission_sequencer
{
enum SequencerState
{
  IDLE,
  PREARM,
  ARM,
  MISSION,
  HOLD,
  LAND,
  DISARM
};

static const char* StateStr[] = { "IDLE", "ARM", "MISSION", "HOLD", "LAND", "DISARM" };

class MissionSequencer
{
  // ROS VARIABLES
private:
  // ROS Node handles
  ros::NodeHandle nh_;   //!< ROS node handle
  ros::NodeHandle pnh_;  //!< ROS private node handle

  // ROS Publishers
  ros::Publisher pub_pose_setpoint_;  //!< ROS publisher for current setpoint
  ros::Publisher pub_ms_response_;  //!< ROS publisher for mission sequencer request's response. This is similar to the
                                    //!< action feedback given once the request has been fullfilled.

  // ROS Subscribers
  ros::Subscriber sub_vehicle_state_;           //!< ROS subscriber for mavros vehicle state
  ros::Subscriber sub_extended_vehicle_state_;  //!< ROS subscriber for extended mavros vehicle state
  ros::Subscriber sub_vehicle_pose_;            //!< ROS subscriber for current vehicle pose
  ros::Subscriber sub_ms_request_;  //!< ROS subscirber for mission sequencer request (ARM, TAKEOFF, MISSION, LAND, etc)
  ros::Subscriber sub_waypoint_file_name_;  //!< ROS subscriber for waypoint file name

  // ROS METHODS
private:
  ///
  /// \brief cbVehicleState ROS topic callback for the mavros vehicle state
  /// \param msg mavros vehicle state
  ///
  void cbVehicleState(const mavros_msgs::State::ConstPtr& msg);

  ///
  /// \brief cbExtendedVehicleState ROS topic callback for the extended mavros vehicle state
  /// \param msg mavros extended vehicle state
  ///
  void cbExtendedVehicleState(const mavros_msgs::ExtendedState::ConstPtr& msg);

  ///
  /// \brief cbPose ROS topic callback for the current vehicle pose
  /// \param msg current vehicle pose in the 'global' navigation frame
  ///
  /// This callback sets the current vehicle pose used for checking waypoint reached in the mission phase.
  /// The first time this function is called the stating_vehicle_pose_ is set to the received pose.
  ///
  void cbPose(const geometry_msgs::PoseStamped::ConstPtr& msg);

  void cbMSRequest(const mission_sequencer::MissionRequest::ConstPtr& msg);
  void cbWaypointFilename(const std_msgs::String::ConstPtr& msg);

  // EXECUTORS
private:
  void performIdle();
  void performArming();
  void performTakeoff();

  ///
  /// \brief performMission
  ///
  void performMission();
  void performHover();
  void performLand();
  void performHold();
  void performDisarming();
  void performAbort();

private:
  bool b_pose_is_valid_{ false };      //!< flag to deterimine if a valid pose has been received
  bool b_state_is_valid_{ false };     //!< flag to determine if a valid mavros state has been received
  bool b_extstate_is_valid_{ false };  //!< flag to determine if a valid extended mavros state has been received

  // state machine
private:
  SequencerState current_sequencer_state_;

  // navigation variables
private:
  geometry_msgs::PoseStamped starting_vehicle_pose_;  //!< determins the start pose of the vehicle
  geometry_msgs::PoseStamped current_vehicle_pose_;   //!< determines the current pose of the vehicle
  geometry_msgs::PoseStamped setpoint_vehicle_pose_;  //!< determines the setpoint (goal) pose of the vehicle

  // REST - WIP
private:
  mavros_msgs::State currentVehicleState_;
  mavros_msgs::ExtendedState currentExtendedVehicleState_;

  int missionID_;
  int requestNumber_;

  std::vector<ParseWaypoint::Waypoint> waypointList_;
  bool reachedWaypoint_;
  ros::Time reachedWaypointTime_;

  mavros_msgs::SetMode offboardMode_;
  mavros_msgs::CommandBool armCmd_;
  mavros_msgs::CommandLong disarmCmd_;
  mavros_msgs::CommandTOL landCmd_;
  ros::Time armRequestTime_;
  ros::Time disarmRequestTime_;
  ros::Time offboardRequestTime_;

  bool relWaypoints_;

  double thresholdPosition_;
  double thresholdYaw_;

  bool landed_;
  bool automatically_land_ = false;
  static const size_t dbg_throttle_rate_ = 10;
  bool verbose_ = false;
  std::string waypoint_fn_ = "";

  ros::ServiceClient rosServiceArm_;
  ros::ServiceClient rosServiceDisrm_;
  ros::ServiceClient rosServiceLand_;
  ros::ServiceClient rosServiceSetMode_;

  /// vector of filenames read from parameter server
  std::vector<std::string> filenames_;

  void publishResponse(int id, int request, bool response, bool completed);

  geometry_msgs::PoseStamped waypointToPoseStamped(const ParseWaypoint::Waypoint& waypoint);

  bool getFilenames();
  bool setFilename(std::string const waypoint_fn);

public:
  MissionSequencer(ros::NodeHandle& nh, ros::NodeHandle& pnh);
  ~MissionSequencer();

  void logic(void);
  void publishPoseSetpoint(void);
};  // class MissionSequencer

}  // namespace mission_sequencer

#endif  // MISSION_SEQUENCER_HPP

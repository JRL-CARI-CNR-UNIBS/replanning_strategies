﻿#include "replanners_lib/replanner_managers/replanner_manager_MARSHA.h"

namespace pathplan
{

ReplannerManagerMARSHA::ReplannerManagerMARSHA(const PathPtr &current_path,
                                               const TreeSolverPtr &solver,
                                               const ros::NodeHandle &nh,
                                               const LengthPenaltyMetricsPtr& ha_metrics,
                                               std::vector<PathPtr> &other_paths):
  ReplannerManagerMARS(current_path,solver,nh,other_paths)
{
  ha_metrics_ = ha_metrics;
  MARSHAadditionalParams();
}

void ReplannerManagerMARSHA::MARSHAadditionalParams()
{
  unaware_obstacles_.clear();
  if(!nh_.getParam("MARSHA/unaware_obstacles",unaware_obstacles_))
    ROS_ERROR("MARSHA/unaware_obstacles_ not set");

  poi_names_.clear();
  if(nh_.getParam("MARSHA/poi_names",poi_names_))
  {
    if(ha_metrics_)
      ha_metrics_->getSSM()->setPoiNames(poi_names_);
  }
  else
    ROS_ERROR("MARSHA/poi_names_ not set");
}

void ReplannerManagerMARSHA::syncPathCost()
{
  ReplannerManagerMARS::syncPathCost();
  ha_metrics_->getSSM()->setObstaclesPositions(obstalces_positions_);
}

void ReplannerManagerMARSHA::startReplannedPathFromNewCurrentConf(const Eigen::VectorXd& configuration)
{
  ReplannerManagerMARS::startReplannedPathFromNewCurrentConf(configuration);
}

void ReplannerManagerMARSHA::initReplanner()
{
  if(not ha_metrics_)
    throw std::runtime_error("HA metrics not defined!");

  double time_for_repl = 0.9*dt_replan_;
  pathplan::MARSHAPtr replanner = std::make_shared<pathplan::MARSHA>(configuration_replan_,current_path_,time_for_repl,
                                                                     solver_,other_paths_,ha_metrics_);

  replanner->reverseStartNodes(reverse_start_nodes_);

  if(full_net_search_)
    ROS_WARN("full net search not available for MARSHA");

  full_net_search_ = false;
  replanner->setFullNetSearch(full_net_search_);

  replanner_ = replanner;

  pathplan::DisplayPtr disp = std::make_shared<pathplan::Display>(planning_scn_cc_,group_name_);
  replanner_->setDisp(disp);
}

Eigen::Matrix <double,3,Eigen::Dynamic> ReplannerManagerMARSHA::updateObstaclesPositions(const moveit_msgs::PlanningSceneWorld& world)
{
  Eigen::Vector3d obstacle_position;
  Eigen::Matrix <double,3,Eigen::Dynamic> obstacles_positions;

  for(const moveit_msgs::CollisionObject& obj:world.collision_objects)
  {
    if(std::find(unaware_obstacles_.begin(),unaware_obstacles_.end(),obj.id)>=unaware_obstacles_.end())
    {
      obstacle_position << obj.pose.position.x,obj.pose.position.y,obj.pose.position.z;

      obstacles_positions.conservativeResize(Eigen::NoChange, obstacles_positions.cols()+1);
      obstacles_positions.col(obstacles_positions.cols()-1) = obstacle_position;
    }
  }

  return obstacles_positions;
}

void ReplannerManagerMARSHA::collisionCheckThread()
{
  moveit_msgs::GetPlanningScene ps_srv;
  Eigen::VectorXd current_configuration_copy;
  moveit_msgs::PlanningScene planning_scene_msg;
  Eigen::Matrix <double,3,Eigen::Dynamic> obstacles_positions;

  ssm15066_estimator::SSM15066Estimator2D ssm_2d_sequential = *(std::static_pointer_cast<ssm15066_estimator::SSM15066Estimator2D>(ha_metrics_->getSSM()->clone()));
  ssm15066_estimator::SSM15066Estimator2DPtr ssm = std::make_shared<ssm15066_estimator::SSM15066Estimator2D>(ssm_2d_sequential);

  //elimina
  const std::type_info& ti1 = typeid(ssm15066_estimator::SSM15066Estimator2D);
  const std::type_info& ti2 = typeid(*ssm);

  if(std::type_index(ti1) != std::type_index(ti2))
  {
    ROS_INFO_STREAM("ssm type "<<ti2.name());
    throw std::runtime_error("ssm not correcly cloned");
  }
  //

  LengthPenaltyMetricsPtr metrics_current_path = std::make_shared<LengthPenaltyMetrics>(ssm);

  PathPtr current_path_copy = current_path_shared_->clone();
  current_path_copy->setChecker(checker_cc_);
  current_path_copy->setMetrics(metrics_current_path);

  std::vector<PathPtr> other_paths_copy;
  std::vector<CollisionCheckerPtr> checkers;
  std::vector<LengthPenaltyMetricsPtr> metrics;

  for(const PathPtr& p:other_paths_shared_)
  {
    PathPtr path_copy = p->clone();

    checkers.push_back(checker_cc_->clone());
    path_copy->setChecker(checkers.back());

    metrics.push_back(std::make_shared<LengthPenaltyMetrics>(ssm->clone()));
    path_copy->setMetrics(metrics.back());

    other_paths_copy.push_back(path_copy);
  }

  int other_path_size = other_paths_copy.size();

  ros::Rate lp(collision_checker_thread_frequency_);
  ros::WallTime tic;

  while((not stop_) && ros::ok())
  {
    tic = ros::WallTime::now();

    /* Update planning scene */
    ps_srv.request.components.components = 20; //moveit_msgs::PlanningSceneComponents::WORLD_OBJECT_GEOMETRY + moveit_msgs::PlanningSceneComponents::ROBOT_STATE_ATTACHED_OBJECTS

    if(not plannning_scene_client_.call(ps_srv))
    {
      ROS_ERROR("call to srv not ok");
      stop_ = true;
      break;
    }

    scene_mtx_.lock();
    planning_scene_msg.world = ps_srv.response.scene.world;
    planning_scene_msg.is_diff = true;

    obstacles_positions = updateObstaclesPositions(planning_scene_msg.world);

    checker_cc_->setPlanningSceneMsg(planning_scene_msg);
    metrics_current_path->getSSM()->setObstaclesPositions(obstacles_positions);

    for(unsigned int i=0;i<checkers.size();i++)
    {
      checkers.at(i)->setPlanningSceneMsg(planning_scene_msg);
      metrics .at(i)->getSSM()->setObstaclesPositions(obstacles_positions);
    }
    scene_mtx_.unlock();

    /* Update paths if they have been changed */
    trj_mtx_.lock();
    paths_mtx_.lock();

    current_configuration_copy = current_configuration_;

    if(current_path_sync_needed_)
    {
      current_path_copy = current_path_shared_->clone();
      current_path_copy->setChecker(checker_cc_);
      current_path_copy->setMetrics(metrics_current_path);
      current_path_sync_needed_ = false;
    }

    other_paths_mtx_.lock();
    if(other_path_size<other_paths_shared_.size())  // if the previous current path has been added, update the vector of copied paths
    {
      assert(other_path_size == (other_paths_shared_.size()-1));

      PathPtr path_copy = other_paths_shared_.back()->clone();

      checkers.push_back(checker_cc_->clone());
      path_copy->setChecker(checkers.back());

      metrics.push_back(std::make_shared<LengthPenaltyMetrics>(ssm->clone()));
      path_copy->setMetrics(metrics.back());

      other_paths_copy.push_back(path_copy);
      other_path_size = other_paths_copy.size();
    }

    for(unsigned int i=0;i<other_paths_shared_.size();i++)  // sync other_paths_shared with its copy
    {
      if(other_paths_sync_needed_.at(i))
      {
        other_paths_copy.at(i) = other_paths_shared_.at(i)->clone();
        other_paths_copy.at(i)->setChecker(checkers.at(i));
        other_paths_copy.at(i)->setMetrics(metrics.at(i));
        other_paths_sync_needed_.at(i) = false;
      }
    }

    other_paths_mtx_.unlock();
    paths_mtx_.unlock();
    trj_mtx_.unlock();

    if((current_configuration_copy-replanner_->getGoal()->getConfiguration()).norm()<goal_tol_)
    {
      stop_ = true;
      break;
    }

    /* Launch collision check tasks */

    std::vector<std::shared_future<bool>> tasks;
    for(unsigned int i=0;i<other_paths_copy.size();i++)
    {
      tasks.push_back(std::async(std::launch::async,
                                 &ReplannerManagerMARSHA::checkPathTask,
                                 this,other_paths_copy.at(i)));
    }

    int conn_idx;
    current_path_copy->findConnection(current_configuration_copy,conn_idx);
    if(conn_idx<0)
      continue;
    else
      current_path_copy->isValidFromConf(current_configuration_copy,conn_idx,checker_cc_);

    for(unsigned int i=0; i<tasks.size();i++)
      tasks.at(i).wait();  //wait for the end of each task

    /* Update the cost of the paths */
    scene_mtx_.lock();
    updatePathsCost(current_path_copy,other_paths_copy);
    planning_scene_msg_.world = ps_srv.response.scene.world;  //not diff,it contains all pln scn info but only world is updated
    planning_scene_diff_msg_ = planning_scene_msg;            //diff, contains only world

    obstalces_positions_ = obstacles_positions;
    scene_mtx_.unlock();

    double duration = (ros::WallTime::now()-tic).toSec();

    if(duration>(1.0/collision_checker_thread_frequency_) && display_timing_warning_)
      ROS_BOLDYELLOW_STREAM("Collision checking thread time expired: total duration-> "<<duration);

    lp.sleep();
  }

  ROS_BOLDCYAN_STREAM("Collision check thread is over");
}

bool ReplannerManagerMARSHA::replan()
{
  assert([&]() ->bool{
           for(PathPtr& p:other_paths_)
           {
             if(p->getMetrics() != ha_metrics_)
             return false;
           }
           if(current_path_->getMetrics() != ha_metrics_)
           return false;

           return true;
         }());

  return ReplannerManagerMARS::replan();
}

}

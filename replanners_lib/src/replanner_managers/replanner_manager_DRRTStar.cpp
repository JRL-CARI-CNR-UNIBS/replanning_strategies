﻿#include "replanners_lib/replanner_managers/replanner_manager_DRRTStar.h"

namespace pathplan
{
ReplannerManagerDRRTStar::ReplannerManagerDRRTStar(const PathPtr &current_path,
                                                   const TreeSolverPtr &solver,
                                                   const ros::NodeHandle &nh):ReplannerManagerBase(current_path,solver,nh)
{
  RRTStarPtr tmp_solver = std::make_shared<pathplan::RRTStar>(solver_->getMetrics(), checker_replanning_, solver_->getSampler());
  tmp_solver->importFromSolver(solver);

  solver_  = tmp_solver;
}

void ReplannerManagerDRRTStar::startReplannedPathFromNewCurrentConf(const Eigen::VectorXd& configuration)
{
//  std::vector<NodePtr> nodes;
//  std::vector<double> costs;
//  detachAddedBranch(nodes,costs);
//  connectCurrentConfToTree();

  PathPtr current_path = replanner_->getCurrentPath();
  PathPtr replanned_path = replanner_->getReplannedPath();
  TreePtr tree = current_path->getTree();

  ConnectionPtr conn;
  NodePtr current_node = current_path->addNodeAtCurrentConfig(configuration,conn,true);

  tree->changeRoot(current_node);

  std::vector<ConnectionPtr> new_conns = tree->getConnectionToNode(replanner_->getGoal());
  replanned_path->setConnections(new_conns);
}

bool ReplannerManagerDRRTStar::haveToReplan(const bool path_obstructed)
{
  return replanIfObstructed(path_obstructed);
}

void ReplannerManagerDRRTStar::initReplanner()
{
  double time_for_repl = 0.9*dt_replan_;
  replanner_ = std::make_shared<pathplan::DynamicRRTStar>(configuration_replan_, current_path_replanning_, time_for_repl, solver_);
}

}

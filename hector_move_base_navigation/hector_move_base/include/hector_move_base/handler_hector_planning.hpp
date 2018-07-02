#ifndef HANDLER_HECTOR_PLANNING_H_
#define HANDLER_HECTOR_PLANNING_H_

#include <hector_nav_core/hector_move_base_handler.h>
#include <hector_nav_core/exploration_planner.h>
#include <pluginlib/class_loader.h>

namespace hector_move_base_handler {

class HectorPlanningHandler : public HectorMoveBaseHandler {
private:
    costmap_2d::Costmap2DROS* costmap_;
    pluginlib::ClassLoader<hector_nav_core::ExplorationPlanner> expl_loader_;
    boost::shared_ptr<hector_nav_core::ExplorationPlanner> exploration_planner_;

    bool isGoalIDEqual(const actionlib_msgs::GoalID& firstGoalID, const actionlib_msgs::GoalID& secondGoalID)
    {
        return ((firstGoalID.stamp == secondGoalID.stamp) && (firstGoalID.id == secondGoalID.id));
    }

    // ------------------------------------------------------------------------------------
    // Contribution by Paul Manns, feature for ARGOS Challenge only.
    // Always plan with SBPL Planner.
    pluginlib::ClassLoader<nav_core::BaseGlobalPlanner> bgp_loader_;
    boost::shared_ptr<nav_core::BaseGlobalPlanner> trajectory_planner_;
    bool sbpl_only;
    // ------------------------------------------------------------------------------------

public:
    HectorPlanningHandler(hector_move_base::IHectorMoveBase* interface) : HectorMoveBaseHandler(interface)
      , expl_loader_("hector_nav_core", "hector_nav_core::ExplorationPlanner")
      , bgp_loader_("nav_core", "nav_core::BaseGlobalPlanner")
    {
        costmap_ = interface->getCostmap();
        ros::NodeHandle private_nh("~");

        private_nh.param("sbpl_only", sbpl_only, false);

        std::string exploration_planner_name = "hector_nav_core_exploration_plugin/HectorNavCoreExplorationPlugin";
        private_nh.param("exploration_planner", exploration_planner_name, exploration_planner_name);


        try {
            //check if a non fully qualified name has potentially been passed in
            if(!expl_loader_.isClassAvailable(exploration_planner_name)){
                std::vector<std::string> classes = expl_loader_.getDeclaredClasses();
                for(unsigned int i = 0; i < classes.size(); ++i){
                    if(exploration_planner_name == expl_loader_.getName(classes[i])){
                        //if we've found a match... we'll get the fully qualified name and break out of the loop
                        ROS_WARN("Planner specifications should now include the package name. You are using a deprecated API. Please switch from %s to %s in your yaml file.",
                                 exploration_planner_name.c_str(), classes[i].c_str());
                        exploration_planner_name = classes[i];
                        break;
                    }
                }
            }

            exploration_planner_ = expl_loader_.createInstance(exploration_planner_name);
            exploration_planner_->initialize(expl_loader_.getName(exploration_planner_name), costmap_);
        } catch (const pluginlib::PluginlibException& ex)
        {
            ROS_FATAL("Failed to create the %s planner, are you sure it is properly registered and that the containing library is built? Exception: %s", exploration_planner_name.c_str(), ex.what());
            exit(1);
        }

        // ------------------------------------------------------------------------------------
        // Contribution by Paul Manns, feature for ARGOS Challenge only.
        // Always plan with SBPL Planner.
        std::string path_planner = "SBPLLatticePlanner";
        private_nh.param("path_planner", path_planner, path_planner);

        try
        {
            //check if a non fully qualified name has potentially been passed in
            if(!bgp_loader_.isClassAvailable(path_planner)){
                std::vector<std::string> classes = bgp_loader_.getDeclaredClasses();
                for(unsigned int i = 0; i < classes.size(); ++i){
                    if(path_planner == bgp_loader_.getName(classes[i])){
                        //if we've found a match... we'll get the fully qualified name and break out of the loop
                        ROS_WARN("Planner specifications should now include the package name. You are using a deprecated API. Please switch from %s to %s in your yaml file.",
                                 path_planner.c_str(), classes[i].c_str());
                        path_planner = classes[i];
                        break;
                    }
                }
            }
            trajectory_planner_ = bgp_loader_.createInstance(path_planner);
            trajectory_planner_->initialize(bgp_loader_.getName(path_planner), costmap_);
        }
        catch (const pluginlib::PluginlibException& ex)
        {
            ROS_FATAL("[move_base] [handler_planning] Failed to create the %s planner. "
                      "Are you sure it is properly registered and the containing library is built? "
                      "Exception: %s", path_planner.c_str(), ex.what());
            exit(1);
        }
        // ------------------------------------------------------------------------------------
    }

    hector_move_base::RESULT handle()
    {
        ROS_DEBUG("[move_base] [planning_handler] Planning started.");

        hector_move_base::handlerActionGoal current_goal = hectorMoveBaseInterface->getCurrentGoal();
        std::vector<geometry_msgs::PoseStamped> plan;
        if(costmap_ == NULL)
        {
            ROS_ERROR("[move_base] [planning_handler] makePlan failed, costmap is NULL");
            return hector_move_base::FAIL;
        }

        // lock costmap
#ifdef LAYERED_COSTMAP_H_
        boost::unique_lock< boost::shared_mutex > lock(*(costmap_->getCostmap()->getLock()));
#endif // LAYERED_COSTMAP_H_

        //get the starting pose of the robot
        tf::Stamped<tf::Pose> global_pose;
        if(!costmap_->getRobotPose(global_pose)){
            ROS_ERROR("[move_base] [planning_handler] makePlan failed, pose not retrievable from costmap.");
            return hector_move_base::FAIL;
        }

        geometry_msgs::PoseStamped start;
        tf::poseStampedTFToMsg(global_pose, start);

        ROS_DEBUG("[move_base] [planning_handler] sbpl_only = %d", sbpl_only);

        bool fixed = false;

        if(sbpl_only)
        {
            // ------------------------------------------------------------------------------------
            // Contribution by Paul Manns, feature for ARGOS Challenge only.
            // Always plan with SBPL Planner.
            geometry_msgs::PoseStamped goalForTrajectory = current_goal.target_pose;

            std::vector<geometry_msgs::PoseStamped> trajectory;
            if(!(trajectory_planner_->makePlan(start, goalForTrajectory, trajectory)))
            {
                ROS_WARN("[hector_move_base] [planning_handler]: [sbpl_only] Execution of hector planner failed for goal (%.2f, %.2f)",
                         goalForTrajectory.pose.position.x, goalForTrajectory.pose.position.y);
                if (hectorMoveBaseInterface->getGlobalGoal().do_exploration)
                {
                    ROS_INFO("[planning_handler]: In Exploration. Looking for new frontier.");
                    return hector_move_base::ALTERNATIVE;
                }
                return hector_move_base::FAIL;
            }
            plan = trajectory;
            fixed = true;
            // ------------------------------------------------------------------------------------
        }
        else
        {
            //if the planner fails or returns a zero length plan, planning failed
            if((!exploration_planner_->makePlan(start, current_goal.target_pose, plan, current_goal.distance)) || plan.empty()){
                ROS_WARN("[move_base] [planning_handler] Execution of hector planner failed for goal (%.2f, %.2f).",
                         current_goal.target_pose.pose.position.x, current_goal.target_pose.pose.position.y);
                if (hectorMoveBaseInterface->getGlobalGoal().do_exploration)
                {
                    ROS_INFO("[move_base] [planning_handler] In Exploration, looking for new frontier.");
                    return hector_move_base::ALTERNATIVE;
                }
                return hector_move_base::FAIL;
            }
            fixed = false;
        }

        hector_move_base_msgs::MoveBaseActionPath new_path = hector_move_base_msgs::MoveBaseActionPath();
        new_path.goal_id = current_goal.goal_id;
        new_path.header.frame_id = current_goal.target_pose.header.frame_id;
        new_path.header.stamp = current_goal.target_pose.header.stamp;
        new_path.goal.fixed = fixed;
        new_path.goal.speed = current_goal.speed;
        new_path.goal.target_path.header.frame_id = new_path.header.frame_id ;
        new_path.goal.target_path.header.stamp = ros::Time::now() ;
        new_path.goal.target_path.poses = plan;
        new_path.reverse_allowed = current_goal.reverse_allowed;

	// Gabriel Hüttenberger: Test for easier robot orientation in front of Victims
	// new_path.goal.target_path.poses[(unsigned int)(plan.size()-1)].orientation = new_path.goal.target_path.poses[(unsigned int)(plan.size()-2)].orientation;

        hectorMoveBaseInterface->setActionPath(new_path);

        ROS_DEBUG("[move_base] [planning_handler] Generated plan, continue with NEXT.");

        if (isGoalIDEqual(current_goal.goal_id, hectorMoveBaseInterface->getCurrentGoal().goal_id)) {
            //hectorMoveBaseInterface->popCurrentGoal();
            //current_goal.target_pose.pose = plan.back().pose;
            //hectorMoveBaseInterface->pushCurrentGoal(current_goal);
        }

        return hector_move_base::NEXT;
    }

    void abort()
    {
        ROS_WARN("[move_base] [planning_handler] Abort was called in planning, this seams to lead to unresponsive behavior");
    }
};
}
#endif

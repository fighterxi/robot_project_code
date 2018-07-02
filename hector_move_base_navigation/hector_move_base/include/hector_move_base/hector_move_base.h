#ifndef HECTOR_MOVE_BASE_H_
#define HECTOR_MOVE_BASE_H_

#include <hector_move_base/handler_hector_exploration.hpp>
#include <hector_move_base/handler_hector_planning.hpp>
#include <hector_move_base/handler_hector_refine_plan.hpp>
#include <hector_move_base/handler_hector_publish_path.hpp>
#include <hector_move_base/handler_hector_wait_for_replanning.hpp>
#include <hector_move_base/handler_hector_wait_for_reexploring.hpp>
#include <hector_move_base/handler_hector_publish_feedback.hpp>
#include <hector_move_base/handler_hector_publish_abort.hpp>
#include <hector_move_base/handler_hector_publish_preempted.hpp>
#include <hector_move_base/handler_hector_publish_rejected.hpp>
#include <hector_move_base/handler_hector_publish_success.hpp>
#include <hector_move_base/handler_hector_stuck_recovery.hpp>
#include <hector_move_base/handler_hector_idle.hpp>
#include <hector_move_base/hector_move_base_state_machine.h>

#include <hector_move_base_msgs/MoveBaseAction.h>
#include <hector_move_base_msgs/MoveBaseActionExplore.h>
#include <hector_move_base_msgs/MoveBaseActionGoal.h>
#include <hector_move_base_msgs/MoveBaseActionPath.h>
#include <hector_move_base_msgs/MoveBaseActionResult.h>
#include <hector_nav_core/hector_move_base_handler.h>
#include <hector_nav_core/hector_move_base_interface.h>

#include <monstertruck_msgs/SetAlternativeTolerance.h>

#include <visualization_msgs/Marker.h>

#include <actionlib/server/simple_action_server.h>
#include <actionlib/client/simple_action_client.h>
#include <ros/ros.h>
#include <std_msgs/String.h>
#include <sensor_msgs/JointState.h>
#include <tf/tf.h>
#include <tf/transform_listener.h>

namespace hector_move_base {

/**
 * @class HectorMoveBase
 * @brief A class that moves the robot base to a goal location.
 */
class HectorMoveBase : public IHectorMoveBase
{
private:
    costmap_2d::Costmap2DROS* costmap_;
    ros::NodeHandle nh_, private_nh_;
    tf::TransformListener& tf_;

    boost::shared_ptr<HectorMoveBaseStateMachine> statemachine_;
    boost::shared_ptr<hector_move_base_handler::HectorMoveBaseHandler> idleState_;
    boost::shared_ptr<hector_move_base_handler::HectorMoveBaseHandler> exploringState_, planningState_, refinePlanState_, publishPathState_, publishFeedbackState_, waitForReplanningState_, waitForReexploringState_;
    boost::shared_ptr<hector_move_base_handler::HectorMoveBaseHandler> publishSuccessState_, publishAbortState_, publishPreemptedState_, publishRejectedState_;
    boost::shared_ptr<hector_move_base_handler::HectorMoveBaseHandler> stuckExplorationRecoveryState_, stuckPlanningRecoveryState_;
    boost::shared_ptr<hector_move_base_handler::HectorMoveBaseHandler> currentState_, nextState_, startState_;

    int goal_id_counter_;
    double goalReachedRadius_, observeLinearTolerance_, observeAngularTolerance_;
    std::string controller_namespace_;
    bool use_alternate_planner_;
    std::vector<handlerActionGoal> goals_;
    hector_move_base_msgs::MoveBaseActionPath path_;
    ros::Publisher current_goal_pub_, drivepath_pub_, feedback_pub_, result_pub_, goalmarker_pub_, footprint_pub_;
    ros::Subscriber cancel_sub_, controller_result_sub_, explore_sub_, goal_sub_, observation_sub_, syscommand_sub_, simple_goal_sub_;
    ros::ServiceClient tolerance_client_;
    pluginlib::ClassLoader<nav_core::RecoveryBehavior> move_base_plugin_loader_;
    std::vector<boost::shared_ptr<nav_core::RecoveryBehavior> > move_base_plugins_;

    bool use_exploring_;

protected:
    void setupStateMachine();

    actionlib::SimpleActionServer<hector_move_base_msgs::MoveBaseAction> action_server_;

public:
    /**
     * @brief  Constructor for the actions
     * @param name The name of the action
     * @param tf A reference to a TransformListener
     */
    HectorMoveBase(std::string name, tf::TransformListener& tf);

    /**
     * @brief  Destructor - Cleans up
     */
    virtual ~HectorMoveBase();

    handlerActionGoal getGlobalGoal();
    handlerActionGoal getCurrentGoal();
    void popCurrentGoal();
    void pushCurrentGoal(const handlerActionGoal&goal);
    void sendActionGoal(const handlerActionGoal&goal);

    hector_move_base_msgs::MoveBaseActionPath getCurrentActionPath();
    void setActionPath (hector_move_base_msgs::MoveBaseActionPath);
    void sendActionPath(const hector_move_base_msgs::MoveBaseActionPath&);
    void setActionServerCanceled();

    void setNextState(boost::shared_ptr<hector_move_base_handler::HectorMoveBaseHandler>);

    costmap_2d::Costmap2DROS* getCostmap();
    tf::TransformListener& getTransformListener();

    void moveBaseStep();

private:
    void loadDefaultMoveBasePlugins();

    /**
     * callback methods
     */
    void exploreCB(const hector_move_base_msgs::MoveBaseActionExploreConstPtr &goal);
    void asGoalCB();
    void observationCB(const hector_move_base_msgs::MoveBaseActionGoalConstPtr &goal);
    void simple_goalCB(const geometry_msgs::PoseStampedConstPtr &simpleGoal);
    void cmd_velCB(const ros::MessageEvent<geometry_msgs::Twist> &event);
    void asCancelCB();
    void syscommandCB(const std_msgs::StringConstPtr &string);
    void controllerResultCB(const hector_move_base_msgs::MoveBaseActionResultConstPtr &result);

    void abortedGoal();
    void preemptedGoal();
    void rejectedGoal();
    void recoveryGoal();
    void successGoal();
    void clearGoal();
    geometry_msgs::PoseStamped goalToGlobalFrame(const geometry_msgs::PoseStamped&);
    bool isGoalIDEqual(const actionlib_msgs::GoalID&,const actionlib_msgs::GoalID&);
};
}
#endif


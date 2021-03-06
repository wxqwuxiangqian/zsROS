//zs: publish a simple goal msg to the node move_base
#include <ros/ros.h>
#include "geometry_msgs/Pose.h"
#include <geometry_msgs/PoseStamped.h>
#include "tf/tf.h"
#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include "std_srvs/Empty.h"

#define KEYCODE_1 0x31
#define KEYCODE_2 0X32
#define KEYCODE_3 0x33
#define KEYCODE_4 0x34
#define KEYCODE_5 0x35
#define KEYCODE_6 0x36
#define KEYCODE_7 0x37

#define KEYCODE_R 0x43
#define KEYCODE_L 0x44
#define KEYCODE_U 0x41
#define KEYCODE_D 0x42
#define KEYCODE_Q 0x71
#define KEYCODE_SPACE 0x20

class zsGoalPubRosAria
{
public:
    zsGoalPubRosAria();
    void keyLoop();
private:
    ros::NodeHandle nh_;
    double x_offset_;
    double y_offset_;
    ros::Publisher pose_pub_;
    ros::Publisher goal_pub_;
    ros::ServiceClient cancel_goal_srv_client_;
    std_srvs::Empty srv_;
    ros::Publisher zs_pose_pub_;
    geometry_msgs::Pose pose_;
};
zsGoalPubRosAria::zsGoalPubRosAria():
        x_offset_(0.0),
        y_offset_(0.0),
        srv_()
{
    std::cout << "zsGoalPubRosAria constructing..." << std::endl;
    nh_.param("x_offset", x_offset_, x_offset_);
    pose_pub_ = nh_.advertise<geometry_msgs::PoseStamped>("move_base_simple/goal", 1);
    cancel_goal_srv_client_ = nh_.serviceClient<std_srvs::Empty>("/zsProxy/cancel_goal");
    zs_pose_pub_ = nh_.advertise<geometry_msgs::Pose>("zs_pose", 1);
    goal_pub_ = nh_.advertise<geometry_msgs::Pose>("zs_goal", 1);
}
int kfd = 0;
struct termios cooked, raw;
void quit(int sig)
{
    tcsetattr(kfd, TCSANOW, &cooked);
    ros::shutdown();
    exit(0);
}
int main(int argc, char** argv)
{
    ros::init(argc, argv, "zsGoalPub_RosAria");
    zsGoalPubRosAria zsGoalPubRosAria;
    signal(SIGINT,quit);
    zsGoalPubRosAria.keyLoop();
    return(0);
}
void zsGoalPubRosAria::keyLoop()
{
    char c;
    bool dirty=false;
    // get the console in raw mode
    tcgetattr(kfd, &cooked);
    memcpy(&raw, &cooked, sizeof(struct termios));
    raw.c_lflag &=~ (ICANON | ECHO);
    // Setting a new line, then end of file
    raw.c_cc[VEOL] = 1;
    raw.c_cc[VEOF] = 2;
    tcsetattr(kfd, TCSANOW, &raw);
    puts("Reading from keyboard");
    puts("---------------------------");
    puts("Use Space key to stop the moving.");
    puts("Use Left key to change the framework");
    puts("Press q to stop the program");
    for(;;)
    {
        // get the next event from the keyboard
        if(read(kfd, &c, 1) < 0)
        {
            perror("read():");
            exit(-1);
        }
//        linear_=angular_=0;
        ROS_DEBUG("value: 0x%02X\n", c);
        switch(c)
        {
           case KEYCODE_L:
                ROS_INFO("You push the left key, change pose to (3.14, 3.14), do not move robot");
                pose_.position.x = 3.14;
                pose_.position.y = 3.14;
                tf::quaternionTFToMsg(tf::createQuaternionFromYaw(90*M_PI/180), pose_.orientation);
                zs_pose_pub_.publish(pose_);
                break;
            case KEYCODE_R:
                ROS_DEBUG("RIGHT");
//                x_offset_ += 0.5;
                x_offset_ += 1.0;
                ROS_INFO_STREAM("You push the RIGHT Arrow!, will go right for 1m, move the robot");
                dirty = true;
                break;
            case KEYCODE_U:
                ROS_INFO_STREAM("You push the UP Arrow key, will go up for 1 m, move the robot");
                y_offset_+=1.0;
                dirty=true;
                break;
            case KEYCODE_D:
                ROS_INFO_STREAM("You push the down key, will go to original(0, 0), move the robot");
                x_offset_=0.0;
                y_offset_=0.0;
                dirty=true;
                break;
            case KEYCODE_SPACE:
                ROS_INFO("zs: You push the SPACE button!");
                if (cancel_goal_srv_client_.call(srv_)){
                    ROS_INFO("zs: send cancel_goal service!");
                }
                break;
            case KEYCODE_Q:
                ROS_DEBUG("QUIT");
                ROS_INFO_STREAM("You quit the program successfully");
                return;
                break;
        }
        geometry_msgs::Pose zsGoal;
        zsGoal.position.x = x_offset_;
        zsGoal.position.y = y_offset_;
        zsGoal.position.z = 0.0;
        // zsGoal.header.stamp = ros::Time::now();
        // zsGoal.header.frame_id = "odom";
        // zsGoal.pose.position.x = x_offset_;
        // zsGoal.pose.position.y = 0.0;
        // zsGoal.pose.position.z = 0.0;
        //zs:
        // tf::quaternionTFToMsg(tf::createQuaternionFromYaw(0.0*M_PI/180), zsGoal.pose.orientation);
        tf::quaternionTFToMsg(tf::createQuaternionFromYaw(0.0*M_PI/180), zsGoal.orientation);
//        twist.angular.z = a_scale_*angular_;
//        twist.linear.x = l_scale_*linear_;
        if(dirty == true)//
        {
            goal_pub_.publish(zsGoal);
            ROS_INFO("zs: published a move_base_action_goal msg and offset equals to: %lf", x_offset_);
            // pose_pub_.publish(zsGoal);
            // ROS_INFO("zs: published a move_base_simple/goal msg and offset equals to: %lf", x_offset_);
            dirty=false;
        }
    }
    return;
}

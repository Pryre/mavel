#pragma once
#include <pidController/pidController.h>

#include <ros/ros.h>

#include <tf2/transform_datatypes.h>
#include <tf2_ros/transform_listener.h>

#include <geometry_msgs/TransformStamped.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/TwistStamped.h>
#include <geometry_msgs/AccelStamped.h>

#include <string>

//PID-based position, velocity and accleration controller for a UAV
//Output calculated with normalized thrust vector and orientation.
//	Note: If either external velocity or acceleration input is used,
//	then a z body rate is sent instead of a orientation (angular
//	acceleration around the z axis are integrated).
//
//		Position Control (Optional: set timout to 0.0):
//			Reference:
//				- Pose message
//				- TF frame
//			Goal:
//				- Pose message
//				- TF frame
//			Feedback:
//				- Pose message
//				- TF frame
//		Velocity Control (Optional: set timout to 0.0):
//			Reference:
//				- Twist message
//			Goal:
//				- Twist message
//			Feedback:
//				- Twist message
//		Acceleration control options:
//			Goal:
//				- Accel message
//			Feedback:
//				- Accel message

enum mavel_data_stream_states {
	HEALTH_OK = 0,
	HEALTH_TIMOUT,
	HEALTH_UNKNOWN
};

template<typename streamDataT> struct mavel_data_stream {
	bool state;				//Whether the stream is reliable

	ros::Duration timeout;	//How long to wait before a timeout
	uint64_t count;			//The current data count since last timeout
	uint64_t stream_count;	//The data count needed to establish a stream

	std::string stream_topic;	//Name of the data stream (for console output)
	streamDataT data;		//Latest data from the stream
};

struct mavel_params_pid {
	double kp;
	double ki;
	double kd;

	double tau;

	double min;
	double max;
};

class Mavel {
	private:
		ros::NodeHandle nh_;

		tf2_ros::Buffer tfbuffer_;
		tf2_ros::TransformListener tfln_;

		ros::Publisher pub_output_position_;
		ros::Publisher pub_output_velocity_;
		ros::Publisher pub_output_acceleration_;
		ros::Publisher pub_output_attitude_;

		ros::Subscriber sub_reference_position_;
		ros::Subscriber sub_reference_velocity_;
		ros::Subscriber sub_setpoint_position_;
		ros::Subscriber sub_setpoint_velocity_;
		ros::Subscriber sub_setpoint_acceleration_;

		ros::Timer timer_controlller_;

		double param_rate_control_;
		double param_tilt_max_;
		double param_throttle_min_;
		double param_throttle_mid_;
		double param_throttle_max_;

		//Rate in Hz
		//Required stream count is derived as 2*rate
		double param_stream_min_rate_position_reference_;
		double param_stream_min_rate_velocity_reference_;
		double param_stream_min_rate_position_setpoint_;
		double param_stream_min_rate_velocity_setpoint_;
		double param_stream_min_rate_acceleration_setpoint_;

		mavel_data_stream<geometry_msgs::PoseStamped> stream_reference_position_;
		mavel_data_stream<geometry_msgs::TwistStamped> stream_reference_velocity_;
		mavel_data_stream<geometry_msgs::PoseStamped> stream_setpoint_position_;
		mavel_data_stream<geometry_msgs::TwistStamped> stream_setpoint_velocity_;
		mavel_data_stream<geometry_msgs::AccelStamped> stream_setpoint_acceleration_;

		mavel_params_pid param_pid_pos_x;
		mavel_params_pid param_pid_pos_y;
		mavel_params_pid param_pid_pos_z;
		mavel_params_pid param_pid_vel_x;
		mavel_params_pid param_pid_vel_y;
		mavel_params_pid param_pid_vel_z;

		pidController controller_pos_x;
		pidController controller_pos_y;
		pidController controller_pos_z;
		pidController controller_vel_x;
		pidController controller_vel_y;
		pidController controller_vel_z;

		double integrator_body_rate_z_;

		std::string param_control_frame_id_;

		std::string topic_input_position_reference_;
		std::string topic_input_velocity_reference_;

		std::string topic_input_position_setpoint_;
		std::string topic_input_velocity_setpoint_;
		std::string topic_input_acceleration_setpoint_;

		std::string topic_output_position_;
		std::string topic_output_velocity_;
		std::string topic_output_acceleration_;
		std::string topic_output_attitude_;

	public:
		Mavel( void );

		~Mavel( void );

		//Handles all the TF requests as set by the params
		//Calls the reference and setpoint callbacks to handle the actual data contained
		void tf_cb( const ros::TimerEvent& );

		void reference_position_cb( const geometry_msgs::PoseStamped msg_in );
		void reference_velocity_cb( const geometry_msgs::TwistStamped msg_in );

		void setpoint_position_cb( const geometry_msgs::PoseStamped msg_in );
		void setpoint_velocity_cb( const geometry_msgs::TwistStamped msg_in );
		void setpoint_acceleration_cb( const geometry_msgs::AccelStamped msg_in );

		//These will hanlde controller steps and publishing any feedback data
		void controller_position_cb( const ros::TimerEvent& );	//Generates a velocity setpoint
		void controller_velocity_cb( const ros::TimerEvent& );	//Generates an acceleration setpoint
		void controller_acceleration_cb( const ros::TimerEvent& );	//Generates the attitude target

	private:
		//Initializes the pid parameters for a controller
		//param_base_name should be set first (e.g. "~/pid/position/")
		void param_pid_init( mavel_params_pid* pid, std::string controller_name );

		//Initializes the stream parameters
		template<typename streamDataT>
		void stream_init( mavel_data_stream<streamDataT>* stream, double min_rate, std::string topic );

		//Updates the stream information that new data has been recieved
		//New data should have already been added into the stream
		template<typename streamDataT>
		void stream_update( mavel_data_stream<streamDataT>* stream, <streamDataT>* data );

		//Checks the stream for a timeout
		template<typename streamDataT>
		mavel_data_stream_states stream_check( mavel_data_stream<streamDataT>* stream, ros::Time time_now );
};

/**
 * @file CD_utils_test.cpp
 * @brief This file contains the unit tests for utility methods in CircleDetetor class
 *
 * @author Atabak Hafeez [atabakhafeez]
 * @author Maria Ficiu [MariaFiciu]
 * @author Rubin Deliallisi [rdeliallisi]
 * @author Siddharth Shukla [thunderboltsid]
 * @bug No known bugs.
 */

#include <gtest/gtest.h>
#include <ros/ros.h>
#include "circle_detector.h"


TEST(CircleDetectorUtilities, ConvertCartesianToScreenCoordinates) {
	int test_inp_x[] = {2, 3, 4};
	int test_inp_y[] = {2, 3, 4};
	int test_out_x[] = {202, 203, 204};
	int test_out_y[] = {98, 97, 96};
	int screen_w = 400;
	int screen_h = 200;
	CircleDetector circle_detector;
	for (int i = 0; i < 3; ++i) {
		circle_detector.ConvertCartesianToScreen(test_inp_x[i], test_inp_y[i], screen_w, screen_h);
		ASSERT_EQ(test_out_x[i], test_inp_x[i]);
		ASSERT_EQ(test_out_y[i], test_inp_y[i]);
	}
}

TEST(CircleDetectorUtilities, ConvertLaserScanToCartesian) {
	float test_inp_range[] = {1.5, 2, 0.3};
	float test_inp_angle[] = {21.3, 42, 69};
	int test_inp_x[] = {0, 0, 0};
	int test_inp_y[] = {0, 0, 0};
	int test_out_x[] = {95, -183, -3};
	int test_out_y[] = {-115, -79, 29};
	CircleDetector circle_detector;
	for (int i = 0; i < 3; ++i) {
		circle_detector.ConvertLaserScanToCartesian(test_inp_x[i], test_inp_y[i], test_inp_range[i], test_inp_angle[i]);
		ASSERT_EQ(test_out_x[i], test_inp_x[i]);
		ASSERT_EQ(test_out_y[i], test_inp_y[i]);
	}
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	ros::init(argc, argv, "circle_detector_utilities");
	return RUN_ALL_TESTS();
}

#!/usr/bin/env python

#Service Provider for the body forces calculation routine

import rospy
from math import pow
from math import pi
from math import sin
from math import cos
from last_letter.srv import *
import tf.transformations
from geometry_msgs.msg import Vector3
from nav_msgs.msg import Odometry


def calc_force_callback(req):
	
	#rospy.loginfo('Entered callback')#
	states = req.states
	inputs = req.inputs

	#Request air data from getAirData server
	airdata = getAirData(states)
	airspeed = airdata.airspeed
	alpha = airdata.alpha
	beta = airdata.beta
	#rospy.loginfo('airspeed:%g, alpha:%g, beta:%g',airspeed, alpha, beta)#

	#Split Control input values
	deltaa = inputs[0]
	deltae = inputs[1]
	deltat = inputs[2]
	deltar = inputs[3]

	#Request Lift and Drag alpha-coefficients from the corresponding server
	c_lift_a = getLiftAlpha(alpha)
	c_drag_a = getDragAlpha(alpha)
	#rospy.loginfo('lift_a:%g, drag_a:%g',c_lift_a, c_drag_a)#

	#Read from parameter server
	rho = rospy.get_param('world/rho')
	g = rospy.get_param('world/g')
	m = rospy.get_param('airframe/m')
	c_lift_q = rospy.get_param('airframe/c_lift_q')	
	c_lift_deltae = rospy.get_param('airframe/c_lift_deltae')
	c_drag_q = rospy.get_param('airframe/c_drag_q')
	c_drag_deltae = rospy.get_param('airframe/c_drag_deltae')
	c = rospy.get_param('airframe/c')
	b = rospy.get_param('airframe/b')
	s = rospy.get_param('airframe/s')
	c_y_0 = rospy.get_param('airframe/c_y_0')
	c_y_b = rospy.get_param('airframe/c_y_b')
	c_y_p = rospy.get_param('airframe/c_y_p')
	c_y_r = rospy.get_param('airframe/c_y_r')
	c_y_deltaa = rospy.get_param('airframe/c_y_deltaa')
	c_y_deltar = rospy.get_param('airframe/c_y_deltar')
	s_prop = rospy.get_param('motor/s_prop')
	c_prop = rospy.get_param('motor/c_prop')
	k_motor = rospy.get_param('motor/k_motor')

	#Convert coefficients to the body frame	
	c_x_a = -c_drag_a*cos(alpha)-c_lift_a*sin(alpha)	
	c_x_q = -c_drag_q*cos(alpha)-c_lift_q*sin(alpha)
	c_x_deltae = -c_drag_deltae*cos(alpha)-c_lift_deltae*sin(alpha)
	c_z_a = -c_drag_a*sin(alpha)-c_lift_a*cos(alpha)
	c_z_q = -c_drag_q*sin(alpha)-c_lift_q*cos(alpha)
	c_z_deltae = -c_drag_deltae*sin(alpha)-c_lift_deltae*cos(alpha)

	#Read orientation and convert to Euler angles
	q1 = states.pose.pose.orientation.x
	q2 = states.pose.pose.orientation.y
	q3 = states.pose.pose.orientation.z
	q4 = states.pose.pose.orientation.w
	(phi, theta, psi) = tf.transformations.euler_from_quaternion([q1,q2,q3,q4])
	#rospy.logerr('%g, %g, %g', phi, theta, psi)

	#Read angular rates 
	p = states.twist.twist.angular.x
	q = states.twist.twist.angular.y
	r = states.twist.twist.angular.z
	
	#Calculate Gravity force
	gx = -m*g*sin(theta)
	gy = m*g*cos(theta)*sin(phi)
	gz = m*g*cos(theta)*cos(phi)

	#Calculate Aerodynamic force
	qbar = 1.0/2.0*rho*pow(airspeed,2)*s #Calculate dynamic pressure
	#rospy.logerr('qbar:%g, rho:%g, s:%g, airspeed:%g',qbar, rho, s, airspeed)
	if airspeed==0:
		(ax, ay, az) = (0, 0, 0)
	else:
		ax = qbar*(c_x_a + c_x_q*c*q/(2*airspeed) + c_x_deltae*deltae)
		ay = qbar*(c_y_0 + c_y_b*beta + c_y_p*b*p/(2*airspeed) + c_y_r*b*r/(2*airspeed) + c_y_deltaa*deltaa + c_y_deltar*deltar)
		az = qbar*(c_z_a + c_z_q*c*q/(2*airspeed) + c_z_deltae*deltae)
	#rospy.logerr('c_x_a:%g, c_z_a:%g',c_x_a, c_z_a)#
	#rospy.logerr('ax:%g, ay:%g, az:%g', ax, ay, az)#
	
	#Calculate Thrust force
	tx = 1.0/2.0*rho*s_prop*c_prop*(pow(k_motor*deltat,2)-pow(airspeed,2))
	ty = 0
	tz = 0

	#Sum forces
	fx = gx+ax+tx
	fy = gy+ay+ty
	fz = gz+az+tz
	
	response = Vector3(fx, fy, fz)
	
	return response

def getAirData(Odo):
    rospy.wait_for_service('calc_air_data')
    try:
		calcAirData = rospy.ServiceProxy('calc_air_data',calc_air_data)
		return calcAirData(Odo.twist.twist.linear)
    except rospy.ServiceException, e:
        rospy.logerr("Service call failed: %s"%e)

def getLiftAlpha(alpha):
	rospy.wait_for_service('c_lift_a')
	try:
		calcLiftAlpha = rospy.ServiceProxy('c_lift_a',c_lift_a)
		return calcLiftAlpha(alpha).c_lift_a
	except rospy.ServiceException, e:
		rospy.logerr("Service call failed: %s"%e)

def getDragAlpha(alpha):
	rospy.wait_for_service('c_drag_a')
	try:
		calcDragAlpha = rospy.ServiceProxy('c_drag_a',c_drag_a)
		return calcDragAlpha(alpha).c_drag_a
	except rospy.ServiceException, e:
		rospy.logerr("Service call failed: %s"%e)


if __name__=='__main__':
	try:
		rospy.init_node('calc_force_server')
		s = rospy.Service('calc_force', calc_force, calc_force_callback)
		rospy.loginfo('calc_force service running')
		while not rospy.is_shutdown():
			rospy.spin()
				
	except rospy.ROSInterruptException:
		pass	



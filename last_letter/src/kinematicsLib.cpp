//////////////////////////
// Define Kinematics class
//////////////////////////

///////////////////
//Class Constructor
Kinematics::Kinematics(ModelPlane * parent)
{
	parentObj = parent; // ModelPlane class pointer
	forceInput.x = 0;
	forceInput.y = 0;
	forceInput.z = 0;
	torqueInput.x = 0;
	torqueInput.y = 0;
	torqueInput.z = 0;
	// Read kinematics related parameters from parameter server
	if(!ros::param::getCached("airframe/m", mass)) {ROS_FATAL("Invalid parameters for -m- in param server!"); ros::shutdown();}
	double j_x, j_y, j_z, j_xz;
	if(!ros::param::getCached("airframe/j_x", j_x)) {ROS_FATAL("Invalid parameters for -j_x- in param server!"); ros::shutdown();}
	if(!ros::param::getCached("airframe/j_y", j_y)) {ROS_FATAL("Invalid parameters for -j_y- in param server!"); ros::shutdown();}
	if(!ros::param::getCached("airframe/j_z", j_z)) {ROS_FATAL("Invalid parameters for -j_z- in param server!"); ros::shutdown();}
	if(!ros::param::getCached("airframe/j_xz", j_xz)) {ROS_FATAL("Invalid parameters for -j_xz- in param server!"); ros::shutdown();}

	// Calculate inertia matrix...
	double G = j_x*j_z-pow(j_xz,2);
	J[0] = j_x; J[1] = 0; J[2] = -j_xz;
	J[3] = 0; J[4] = j_y; J[5] = 0;
	J[6] = -j_xz; J[7] = 0; J[8] = j_z;
	// ... and its iverse
	Jinv[0] = j_z/G; Jinv[1] = 0; Jinv[2] = j_xz/G;
	Jinv[3] = 0; Jinv[4] = 1/j_y; Jinv[5] = 0;
	Jinv[6] = j_xz/G; Jinv[7] = 0; Jinv[8] = j_x/G;
	// Create a Factory object and make it build an integrator
	Factory factory;
	integrator = factory.buildIntegrator(parentObj);
}

//////////////////
//Class Destructor
Kinematics::~Kinematics()
{
	delete integrator;
	delete parentObj;
}

///////////////////////////////
//State derivatives calculation
void Kinematics::calcDerivatives()
{
	// variable declaration
	geometry_msgs::Vector3 tempVect;
	double Reb[9];
	
	// create transformation matrix from state orientation quaternion
	quat2rotmtx (parentObj->states.pose.orientation, Reb);
	
	// create position derivatives from earth velocity
	posDot = Reb*parentObj->states.velocity.linear;
	
	// create body velocity derivatives from acceleration, angular rotation and body velocity
	geometry_msgs::Vector3 linearAcc = (1.0/mass)*forceInput;
	geometry_msgs::Vector3 corriolisAcc;
	vector3_cross(-parentObj->states.velocity.angular, parentObj->states.velocity.linear, &corriolisAcc);
	speedDot = linearAcc + corriolisAcc;
	
	// create angular derivatives quaternion from angular rates
	quatDot.w = 1.0;
	quatDot.x = parentObj->states.velocity.angular.x*0.5*parentObj->dt;
	quatDot.y = parentObj->states.velocity.angular.y*0.5*parentObj->dt;
	quatDot.z = parentObj->states.velocity.angular.z*0.5*parentObj->dt;
	
	// create angular rate derivatives from torque
	vector3_cross(parentObj->states.velocity.angular, J*parentObj->states.velocity.angular, &tempVect);
	tempVect = -tempVect+torqueInput;
	rateDot = Jinv*tempVect;
}

//////////////////////////
// Define Integrator class
//////////////////////////

Integrator::Integrator(ModelPlane * parent)
{
	parentObj = parent; // ModelPlane class pointer
}

Integrator::~Integrator()
{
	delete parentObj;
}

//////////////////////////
// Define ForwardEuler class
//////////////////////////

//Class Constructor
ForwardEuler::ForwardEuler(ModelPlane * parent) : Integrator(parent)
{
}

//Propagation of the states
void ForwardEuler::propagation()
{
	geometry_msgs::Vector3 tempVect;
	
	// Propagate the NED coordinates from earth velocity
	parentObj->states.pose.position.x = parentObj->states.pose.position.x + parentObj->kinematics.posDot.x * parentObj->dt;
	parentObj->states.pose.position.y = parentObj->states.pose.position.y + parentObj->kinematics.posDot.y * parentObj->dt;
	parentObj->states.pose.position.z = parentObj->states.pose.position.z + parentObj->kinematics.posDot.z * parentObj->dt;
	
	// Propagate orientation quaternion from angular derivatives quaternion
	geometry_msgs::Quaternion quat = parentObj->states.pose.orientation;
	quat_product(quat,parentObj->kinematics.quatDot,&(parentObj->states.pose.orientation));
	quat_normalize(&(parentObj->states.pose.orientation));
	
	// Propagate body velocity from body velocity derivatives
	tempVect = parentObj->dt*parentObj->kinematics.speedDot;
	parentObj->states.velocity.linear = parentObj->states.velocity.linear + tempVect;
	
	// Propagates angular velocity from angular derivatives
	tempVect = parentObj->dt*parentObj->kinematics.rateDot;
	parentObj->states.velocity.angular = parentObj->states.velocity.angular + tempVect;
	
	// Set linear acceleration from the speed derivatives
	parentObj->states.acceleration.linear = parentObj->kinematics.speedDot;
	
	// Set angular acceleration from the angular rate derivatives
	parentObj->states.acceleration.angular = parentObj->kinematics.rateDot;
	
	//Update Geoid stuff -- To update!
	parentObj->states.geoid.altitude = -parentObj->states.pose.position.z;
}
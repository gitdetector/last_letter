// Propulsion class related declarations

// Propulsion interface class declaration
class Propulsion
{
	public:
	///////////
	//Variables
	ModelPlane * parentObj; // pointer to parent ModelPlane class
	double omega; // motor angular speed in rad/s
	geometry_msgs::Wrench wrenchProp;

	///////////
	//Functions	
	Propulsion(ModelPlane *);
	~Propulsion();

	virtual void updateRadPS() =0; // Step the angular speed
	virtual geometry_msgs::Vector3 getForce() =0; // Calculate Forces
	virtual geometry_msgs::Vector3 getTorque() =0; //Calculate Torques
};

// Electric engine model found in R. Beard's book
class EngBeard: public Propulsion
{
	public:
	///////////
	//Variables
	double s_prop, c_prop, k_motor, k_t_p, k_omega;
	double airspeed, rho, deltat;
	
	///////////
	//Functions
	EngBeard(ModelPlane *);
	~EngBeard();
	
	void updateRadPS(); //Step the angular speed
	geometry_msgs::Vector3 getForce(); //Calculate Forces
	geometry_msgs::Vector3 getTorque(); //Calculate Torques
};
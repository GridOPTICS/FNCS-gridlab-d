/** $Id: inverter.cpp,v 1.0 2008/07/15 
	Copyright (C) 2008 Battelle Memorial Institute
	@file inverter.cpp
	@defgroup inverter
	@ingroup generators

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "generators.h"
#include "power_electronics.h"
#include "inverter.h"

#define DEFAULT 1.0;

//CLASS *inverter::plcass = power_electronics;
CLASS *inverter::oclass = NULL;
inverter *inverter::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
inverter::inverter(MODULE *module)
{	
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"inverter",sizeof(inverter),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class inverter";
		else
			oclass->trl = TRL_PROOF;
		
		if (gl_publish_variable(oclass,

			PT_enumeration,"inverter_type",PADDR(inverter_type_v),
				PT_KEYWORD,"TWO_PULSE",(enumeration)TWO_PULSE,
				PT_KEYWORD,"SIX_PULSE",(enumeration)SIX_PULSE,
				PT_KEYWORD,"TWELVE_PULSE",(enumeration)TWELVE_PULSE,
				PT_KEYWORD,"PWM",(enumeration)PWM,
				PT_KEYWORD,"FOUR_QUADRANT",(enumeration)FOUR_QUADRANT,

			PT_enumeration,"four_quadrant_control_mode",PADDR(four_quadrant_control_mode),
				PT_KEYWORD,"NONE",(enumeration)FQM_NONE,
				PT_KEYWORD,"CONSTANT_PQ",(enumeration)FQM_CONSTANT_PQ,
				PT_KEYWORD,"CONSTANT_PF",(enumeration)FQM_CONSTANT_PF,
				//PT_KEYWORD,"CONSTANT_V",FQM_CONSTANT_V,	//Not implemented yet
				//PT_KEYWORD,"VOLT_VAR",FQM_VOLT_VAR,
				PT_KEYWORD,"LOAD_FOLLOWING",(enumeration)FQM_LOAD_FOLLOWING,

			PT_enumeration,"generator_status",PADDR(gen_status_v),
				PT_KEYWORD,"OFFLINE",(enumeration)OFFLINE,
				PT_KEYWORD,"ONLINE",(enumeration)ONLINE,	

			PT_enumeration,"generator_mode",PADDR(gen_mode_v),
				PT_KEYWORD,"UNKNOWN",UNKNOWN,
				PT_KEYWORD,"CONSTANT_V",(enumeration)CONSTANT_V,
				PT_KEYWORD,"CONSTANT_PQ",(enumeration)CONSTANT_PQ,
				PT_KEYWORD,"CONSTANT_PF",(enumeration)CONSTANT_PF,
				PT_KEYWORD,"SUPPLY_DRIVEN",(enumeration)SUPPLY_DRIVEN,

			PT_complex, "V_In[V]",PADDR(V_In),
			PT_complex, "I_In[A]",PADDR(I_In),
			PT_complex, "VA_In[VA]", PADDR(VA_In),
			PT_complex, "VA_Out[VA]", PADDR(VA_Out),
			PT_complex, "Vdc[V]", PADDR(Vdc),
			PT_complex, "phaseA_V_Out[V]", PADDR(phaseA_V_Out),
			PT_complex, "phaseB_V_Out[V]", PADDR(phaseB_V_Out),
			PT_complex, "phaseC_V_Out[V]", PADDR(phaseC_V_Out),
			PT_complex, "phaseA_I_Out[V]", PADDR(phaseA_I_Out),
			PT_complex, "phaseB_I_Out[V]", PADDR(phaseB_I_Out),
			PT_complex, "phaseC_I_Out[V]", PADDR(phaseC_I_Out),
			PT_complex, "power_A[VA]", PADDR(power_A),
			PT_complex, "power_B[VA]", PADDR(power_B),
			PT_complex, "power_C[VA]", PADDR(power_C),
			PT_double, "P_Out[VA]", PADDR(P_Out),
			PT_double, "Q_Out[VAr]", PADDR(Q_Out),
			PT_double, "power_in[W]", PADDR(p_in),
			PT_double, "rated_power[VA]", PADDR(p_rated),
			PT_double, "rated_battery_power[W]", PADDR(bp_rated),
			PT_double, "inverter_efficiency", PADDR(inv_eta),
			PT_double, "battery_soc[pu]", PADDR(b_soc),
			PT_double, "soc_reserve[pu]", PADDR(soc_reserve),
			PT_double, "power_factor[unit]", PADDR(power_factor),

			PT_set, "phases", PADDR(phases),
				PT_KEYWORD, "A",(set)PHASE_A,
				PT_KEYWORD, "B",(set)PHASE_B,
				PT_KEYWORD, "C",(set)PHASE_C,
				PT_KEYWORD, "N",(set)PHASE_N,
				PT_KEYWORD, "S",(set)PHASE_S,
			//multipoint efficiency model parameters'
			PT_bool, "use_multipoint_efficiency", PADDR(use_multipoint_efficiency),
			PT_enumeration, "inverter_manufacturer", PADDR(inverter_manufacturer),
				PT_KEYWORD, "NONE", (enumeration)NONE,
				PT_KEYWORD, "FRONIUS", (enumeration)FRONIUS,
				PT_KEYWORD, "SMA", (enumeration)SMA,
				PT_KEYWORD, "XANTREX", (enumeration)XANTREX,
			PT_double, "maximum_dc_power", PADDR(p_dco),
			PT_double, "maximum_dc_voltage", PADDR(v_dco),
			PT_double, "minimum_dc_power", PADDR(p_so),
			PT_double, "c_0", PADDR(c_o),
			PT_double, "c_1", PADDR(c_1),
			PT_double, "c_2", PADDR(c_2),
			PT_double, "c_3", PADDR(c_3),
			//load following parameters
			PT_object,"sense_object", PADDR(sense_object), PT_DESCRIPTION, "name of the object the inverter is trying to mitigate the load on (node/link) in LOAD_FOLLOWING",
			PT_double,"max_charge_rate[W]", PADDR(max_charge_rate), PT_DESCRIPTION, "maximum rate the battery can be charged in LOAD_FOLLOWING",
			PT_double,"max_discharge_rate[W]", PADDR(max_discharge_rate), PT_DESCRIPTION, "maximum rate the battery can be discharged in LOAD_FOLLOWING",
			PT_double,"charge_on_threshold[W]", PADDR(charge_on_threshold), PT_DESCRIPTION, "power level at which the inverter should try charging the battery in LOAD_FOLLOWING",
			PT_double,"charge_off_threshold[W]", PADDR(charge_off_threshold), PT_DESCRIPTION, "power level at which the inverter should cease charging the battery in LOAD_FOLLOWING",
			PT_double,"discharge_on_threshold[W]", PADDR(discharge_on_threshold), PT_DESCRIPTION, "power level at which the inverter should try discharging the battery in LOAD_FOLLOWING",
			PT_double,"discharge_off_threshold[W]", PADDR(discharge_off_threshold), PT_DESCRIPTION, "power level at which the inverter should cease discharging the battery in LOAD_FOLLOWING",
			PT_double,"excess_input_power[W]", PADDR(excess_input_power), PT_DESCRIPTION, "Excess power at the input of the inverter that is otherwise just lost, or could be shunted to a battery",
			PT_double,"charge_lockout_time[s]",PADDR(charge_lockout_time), PT_DESCRIPTION, "Lockout time when a charging operation occurs before another LOAD_FOLLOWING dispatch operation can occur",
			PT_double,"discharge_lockout_time[s]",PADDR(discharge_lockout_time), PT_DESCRIPTION, "Lockout time when a discharging operation occurs before another LOAD_FOLLOWING dispatch operation can occur",
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
	}
}
/* Object creation is called once for each object that is created by the core */
int inverter::create(void) 
{
	// Default values for Inverter object.
	P_Out = 1200;  // P_Out and Q_Out are set by the user as set values to output in CONSTANT_PQ mode
	Q_Out = 500;
	V_In_Set_A = complex(480,0);
	V_In_Set_B = complex(-240, 415.69);
	V_In_Set_C = complex(-240,-415.69);
	V_Set_A = 240;
	V_Set_B = 240;
	V_Set_C = 240;
	margin = 10;
	I_out_prev = 0;
	I_step_max = 100;
	internal_losses = 0;
	C_Storage_In = 0;
	power_factor = 1;
	
	// Default values for power electronics settings
	Rated_kW = 10;		//< nominal power in kW
	Max_P = 10;			//< maximum real power capacity in kW
	Min_P = 0;			//< minimum real power capacity in kW
	Max_Q = 10;			//< maximum reactive power capacity in kVar
	Min_Q = 10;			//< minimum reactive power capacity in kVar
	Rated_kVA = 15;		//< nominal capacity in kVA
	Rated_kV = 10;		//< nominal line-line voltage in kV
	//Rinternal = 0.035;
	Rload = 1;
	Rtotal = 0.05;
	Rground = 0.03;
	Rground_storage = 0.05;
	Vdc = 480;

	Cinternal = 0;
	Cground = 0;
	Ctotal = 0;
	Linternal = 0;
	Lground = 0;
	Ltotal = 0;
	filter_120HZ = true;
	filter_180HZ = true;
	filter_240HZ = true;
	pf_in = 0;
	pf_out = 1;
	number_of_phases_in = 0;
	phaseAIn = false;
	phaseBIn = false;
	phaseCIn = false;
	phaseAOut = true;
	phaseBOut = true;
	phaseCOut = true;

	last_current[0] = last_current[1] = last_current[2] = last_current[3] = 0.0;
	last_power[0] = last_power[1] = last_power[2] = last_power[3] = 0.0;

	switch_type_choice = IDEAL_SWITCH;
	filter_type_v = (enumeration)BAND_PASS;
	filter_imp_v = (enumeration)IDEAL_FILTER;
	power_in = DC;
	power_out = AC;

	islanded = FALSE;
	use_multipoint_efficiency = FALSE;
	p_dco = -1;
	p_so = -1;
	v_dco = -1;
	c_o = -1;
	c_1 = -1;
	c_2 = -1;
	c_3 = -1;
	p_max = -1;
	pMeterStatus = NULL;
	efficiency = 0;
	inv_eta = 0.0;	//Not sure why there's two of these...

	sense_object = NULL;
	max_charge_rate = 0;
	max_discharge_rate = 0;
	charge_on_threshold = 0;
	charge_off_threshold = 0;
	discharge_on_threshold = 0;
	discharge_off_threshold = 0;
	powerCalc = NULL;
	sense_is_link = false;
	sense_power = NULL;

	excess_input_power = 0.0;
	lf_dispatch_power = 0.0;
	load_follow_status = IDLE;	//LOAD_FOLLOWING starts out doing nothing
	four_quadrant_control_mode = FQM_CONSTANT_PF;	//Four quadrant defaults to constant PF mode

	next_update_time = 0;
	lf_dispatch_change_allowed = true;	//Begins with changle allowed
	charge_lockout_time = 0.0;	//Charge and discharge default to no delay
	discharge_lockout_time = 0.0;

	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int inverter::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("inverter::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	// construct circuit variable map to meter
	static complex default_line123_voltage[3], default_line1_current[3];
	static int default_meter_status;	//Not really a good place to do this, but keep consistent
	int i;

	// find parent meter or triplex_meter, if not defined, use default voltages, and if
	// the parent is not a meter throw an exception
	if (parent!=NULL && gl_object_isa(parent,"meter"))
	{
		// attach meter variables to each circuit
		parent_string = "meter";
		struct {
			complex **var;
			char *varname;
		}
		map[] = {
		// local object name,	meter object name
			{&pCircuit_V,			"voltage_A"}, // assumes 2 and 3 follow immediately in memory
			{&pLine_I,				"current_A"}, // assumes 2 and 3(N) follow immediately in memory
			{&pPower,				"power_A"}, // assumes 2 and 3 follow immediately in memory
		};
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
	
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
			*(map[i].var) = get_complex(parent,map[i].varname);

		//Map status
		pMeterStatus = get_enum(parent,"service_status");

		//Check it
		if (pMeterStatus==NULL)
		{
			GL_THROW("Inverter failed to map powerflow status variable");
			/*  TROUBLESHOOT
			While attempting to map the service_status variable of the parent
			powerflow object, an error occurred.  Please try again.  If the error
			persists, please submit your code and a bug report via the trac website.
			*/
		}

		//Map phases
		set *phaseInfo;
		PROPERTY *tempProp;
		tempProp = gl_get_property(parent,"phases");

		if ((tempProp==NULL || tempProp->ptype!=PT_set))
		{
			GL_THROW("Unable to map phases property - ensure the parent is a meter or triplex_meter");
			/*  TROUBLESHOOT
			While attempting to map the phases property from the parent object, an error was encountered.
			Please check and make sure your parent object is a meter or triplex_meter inside the powerflow module and try
			again.  If the error persists, please submit your code and a bug report via the Trac website.
			*/
		}
		else
			phaseInfo = (set*)GETADDR(parent,tempProp);

		//Copy in so the code works
		phases = *phaseInfo;

	}
	else if (parent!=NULL && gl_object_isa(parent,"triplex_meter"))
	{
		parent_string = "triplex_meter";

		struct {
			complex **var;
			char *varname;
		}
		map[] = {
			// local object name,	meter object name
			{&pCircuit_V,			"voltage_12"}, // assumes 1N and 2N follow immediately in memory
			{&pLine_I,				"current_1"}, // assumes 2 and 3(N) follow immediately in memory
			{&pLine12,				"current_12"}, // maps current load 1-2 onto triplex load
			{&pPower,				"power_12"}, //assumes 2 and 1-2 follow immediately in memory
			/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
		};

		// attach meter variables to each circuit
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
		{
			if ((*(map[i].var) = get_complex(parent,map[i].varname))==NULL)
			{
				GL_THROW("%s (%s:%d) does not implement triplex_meter variable %s for %s (inverter:%d)", 
				/*	TROUBLESHOOT
					The Inverter requires that the triplex_meter contains certain published properties in order to properly connect
					the inverter to the triplex-meter.  If the triplex_meter does not contain those properties, GridLAB-D may
					suffer fatal pointer errors.  If you encounter this error, please report it to the developers, along with
					the version of GridLAB-D that raised this error.
				*/
				parent->name?parent->name:"unnamed object", parent->oclass->name, parent->id, map[i].varname, obj->name?obj->name:"unnamed", obj->id);
			}
		}

		//Map status
		pMeterStatus = get_enum(parent,"service_status");

		//Check it
		if (pMeterStatus==NULL)
		{
			GL_THROW("Inverter failed to map powerflow status variable");
			//Defined above
		}

		//Map phases
		set *phaseInfo;
		PROPERTY *tempProp;
		tempProp = gl_get_property(parent,"phases");

		if ((tempProp==NULL || tempProp->ptype!=PT_set))
		{
			GL_THROW("Unable to map phases property - ensure the parent is a meter or triplex_meter");
			//Defined above
		}
		else
			phaseInfo = (set*)GETADDR(parent,tempProp);

		//Copy in so the code works
		phases = *phaseInfo;
	}
	else if	((parent != NULL && strcmp(parent->oclass->name,"meter") != 0)||(parent != NULL && strcmp(parent->oclass->name,"triplex_meter") != 0))
	{
		throw("Inverter must have a meter or triplex meter as it's parent");
		/*  TROUBLESHOOT
		Check the parent object of the inverter.  The inverter is only able to be childed via a meter or 
		triplex meter when connecting into powerflow systems.  You can also choose to have no parent, in which
		case the inverter will be a stand-alone application using default voltage values for solving purposes.
		*/
	}
	else
	{
		parent_string = "none";
		
		struct {
			complex **var;
			char *varname;
		}
		map[] = {
		// local object name,	meter object name
			{&pCircuit_V,			"voltage_A"}, // assumes 2 and 3 follow immediately in memory
			{&pLine_I,				"current_A"}, // assumes 2 and 3(N) follow immediately in memory
		};

		gl_warning("Inverter:%d has no parent meter object defined; using static voltages", obj->id);
		
		// attach meter variables to each circuit in the default_meter
		*(map[0].var) = &default_line123_voltage[0];
		*(map[1].var) = &default_line1_current[0];

		//Attach meter status default
		pMeterStatus = &default_meter_status;

		// provide initial values for voltages
		default_line123_voltage[0] = complex(Rated_kV*1000/sqrt(3.0),0);
		default_line123_voltage[1] = complex(Rated_kV*1000/sqrt(3.0)*cos(2*PI/3),Rated_kV*1000/sqrt(3.0)*sin(2*PI/3));
		default_line123_voltage[2] = complex(Rated_kV*1000/sqrt(3.0)*cos(-2*PI/3),Rated_kV*1000/sqrt(3.0)*sin(-2*PI/3));
		default_meter_status = 1;

		// Declare all 3 phases
		phases = 0x07;
	}

	// count the number of phases
	if ( (phases & 0x10) == 0x10) // split phase
		number_of_phases_out = 1; 
	else if ( (phases & 0x07) == 0x07 ) // three phase
		number_of_phases_out = 3;
	else if ( ((phases & 0x03) == 0x03) || ((phases & 0x05) == 0x05) || ((phases & 0x06) == 0x06) ) // two-phase
		number_of_phases_out = 2;
	else if ( ((phases & 0x01) == 0x01) || ((phases & 0x02) == 0x02) || ((phases & 0x04) == 0x04) ) // single phase
		number_of_phases_out = 1;
	else
	{
		//Never supposed to really get here
		GL_THROW("Invalid phase configuration specified!");
		/*  TROUBLESHOOT
		An invalid phase congifuration was specified when attaching to the "parent" object.  Please report this
		error.
		*/
	}

	if (gen_mode_v == UNKNOWN)
	{
		gl_warning("Inverter control mode is not specified! Using default: CONSTANT_PF");
		gen_mode_v = (enumeration)CONSTANT_PF;
	}
	if (gen_status_v == UNKNOWN)
	{
		gl_warning("Inverter status is unknown! Using default: ONLINE");
		gen_status_v = (enumeration)ONLINE;
	}
	if (inverter_type_v == UNKNOWN)
	{
		gl_warning("Inverter type is unknown! Using default: PWM");
		inverter_type_v = (enumeration)PWM;
	}
			
			//need to check for parameters SWITCH_TYPE, FILTER_TYPE, FILTER_IMPLEMENTATION, GENERATOR_MODE
	/*
			if (Rated_kW!=0.0)  SB = Rated_kW/sqrt(1-Rated_pf*Rated_pf);
			if (Rated_kVA!=0.0)  SB = Rated_kVA/3;
			if (Rated_kV!=0.0)  EB = Rated_kV/sqrt(3.0);
			if (SB!=0.0)  ZB = EB*EB/SB;
			else throw("Generator power capacity not specified!");
			double Real_Rinternal = Rinternal * ZB; 
			double Real_Rload = Rload * ZB;
			double Real_Rtotal = Rtotal * ZB;
			double Real_Rphase = Rphase * ZB;
			double Real_Rground = Rground * ZB;
			double Real_Rground_storage = Rground_storage * ZB;
			double[3] Real_Rfilter = Rfilter * ZB;

			double Real_Cinternal = Cinternal * ZB;
			double Real_Cground = Cground * ZB;
			double Real_Ctotal = Ctotal * ZB;
			double[3] Real_Cfilter = Cfilter * ZB;

			double Real_Linternal = Linternal * ZB;
			double Real_Lground = Lground * ZB;
			double Real_Ltotal = Ltotal * ZB;
			double[3] Real_Lfilter = Lfilter * ZB;

			tst = complex(Real_Rground,Real_Lground);
			AMx[0][0] = complex(Real_Rinternal,Real_Linternal) + tst;
			AMx[1][1] = complex(Real_Rinternal,Real_Linternal) + tst;
			AMx[2][2] = complex(Real_Rinternal,Real_Linternal) + tst;
		//	AMx[0][0] = AMx[1][1] = AMx[2][2] = complex(Real_Rs+Real_Rg,Real_Xs+Real_Xg);
			AMx[0][1] = AMx[0][2] = AMx[1][0] = AMx[1][2] = AMx[2][0] = AMx[2][1] = tst;

			*/

	//Dump efficiency from inv_eta first - it will get overwritten if it is bad
	efficiency=inv_eta;

	//all other variables set in input file through public parameters
	switch(inverter_type_v)
	{
		case TWO_PULSE:
			if (inv_eta==0)
			{
				efficiency = 0.8;
				gl_warning("Efficiency unspecified - defaulted to %f for this inverter type",efficiency);
				/*  TROUBLESHOOT
				An inverter_efficiency value was not explicitly specified for this inverter.  A default
				value was specified in its place.  If the default value is not acceptable, please explicitly
				set inverter_efficiency in the GLM file.
				*/
			}
			break;
		case SIX_PULSE:
			if (inv_eta==0)
			{
				efficiency = 0.8;
				gl_warning("Efficiency unspecified - defaulted to %f for this inverter type",efficiency);
				//defined above
			}
			break;
		case TWELVE_PULSE:
			if (inv_eta==0)
			{
				efficiency = 0.8;
				gl_warning("Efficiency unspecified - defaulted to %f for this inverter type",efficiency);
				//defined above
			}
			break;
		case PWM:
			if (inv_eta==0)
			{
				efficiency = 0.9;
				gl_warning("Efficiency unspecified - defaulted to %f for this inverter type",efficiency);
				//defined above
			}
			break;
		case FOUR_QUADRANT:
			if(four_quadrant_control_mode == FQM_LOAD_FOLLOWING)
			{
				//Make sure we have an appropriate object to look at, if null, steal our parent
				if (sense_object == NULL)
				{
					if (parent!=NULL)
					{
						//Put the parent in there
						sense_object = parent;
						gl_warning("inverter:%s - sense_object not specified for LOAD_FOLLOWING - attempting to use parent object",obj->name);
						/*  TROUBLESHOOT
						The inverter is currently configured for LOAD_FOLLOWING mode, but did not have an appropriate
						sense_object specified.  The inverter is therefore using the parented object as the expected
						sense_object.
						*/
					}
					else
					{
						gl_error("inverter:%s - LOAD_FOLLOWING will not work without a specified sense_object!",obj->name);
						/*  TROUBLESHOOT
						The inverter is currently configured for LOAD_FOLLOWING mode, but does not have
						an appropriate sense_object specified.  Please specify a proper object and try again.
						*/
						return 0;
					}
				}

				//See what kind of sense_object we are linked at - note that the current implementation only takes overall power
				if (gl_object_isa(sense_object,"node","powerflow"))
				{
					//Make sure it's a meter of some sort
					if (gl_object_isa(sense_object,"meter","powerflow") || gl_object_isa(sense_object,"triplex_meter","powerflow"))
					{
						//Set flag
						sense_is_link = false;

						//Map to measured_power - regardless of object
						sense_power = get_complex(sense_object,"measured_power");

						//Make sure it worked
						if (sense_power == NULL)
						{
							gl_error("inverter:%s - an error occurred while mapping the sense_object power measurement!",obj->name);
							/*  TROUBLEHSHOOT
							While attempting to map the property defining measured power on the sense_object, an error was encountered.
							Please try again.  If the error persists, please submit a bug report and your code via the trac website.
							*/
							return 0;
						}

						//Random warning about ranks, if not our parent
						if (sense_object != parent)
						{
							gl_warning("inverter:%s is LOAD_FOLLOWING based on a meter or triplex_meter, ensure the inverter is connected inline with that object!",obj->name);
							/*  TROUBLESHOOT
							The inverter operates in LOAD_FOLLOWING mode under the assumption the sense_object meter or triplex_meter is either attached to
							the inverter, or directly upstream in the flow.  If this assumption is violated, the results may not be as expected.
							*/
						}
					}
					else	//loads/nodes/triplex_nodes not supported
					{
						gl_error("inverter:%s - sense_object is a node, but not a meter or triplex_meter!",obj->name);
						/*  TROUBLESHOOT
						When in LOAD_FOLLOWING and the sense_object is a powerflow node, that powerflow object
						must be a meter or a triplex_meter.  Please change your model and try again.
						*/
						return 0;
					}
				}
				else if (gl_object_isa(sense_object,"link","powerflow"))
				{
					//Only transformers supported right now (functional link - just needs to be exported elsewhere)
					if (gl_object_isa(sense_object,"transformer","powerflow"))
					{
						//Set flag
						sense_is_link = true;

						//Link up the power_calculation() function
						powerCalc = (FUNCTIONADDR)(gl_get_function(sense_object,"power_calculation"));

						//Make sure it worked
						if (powerCalc==NULL)
						{
							gl_error("inverter:%s - inverter failed to map power calculation function of transformer!",obj->name);
							/*  TROUBLESHOOT
							While attempting to link up the power_calculation function for the transformer specified in sense_object,
							something went wrong.  Please try again.  If the error persists, please post your code and a bug report via
							the trac website.
							*/
							return 0;
						}

						//Map to the property to compare - just use power_out for now (just as good as power_in)
						sense_power = get_complex(sense_object,"power_out");

						//Make sure it worked
						if (sense_power == NULL)
						{
							gl_error("inverter:%s - an error occurred while mapping the sense_object power measurement!",obj->name);
							//Defined above
							return 0;
						}
					}
					else	//Not valid
					{
						gl_error("inverter:%s - sense_object is a link, but not a transformer!",obj->name);
						/*  TROUBLESHOOT
						When in LOAD_FOLLOWING and the sense_object is a powerflow link, that powerflow object
						must be a transformer.  Please change your model and try again.
						*/
						return 0;
					}

				}
				else	//Not a link or a node, we don't know what to do!
				{
					gl_error("inverter:%s - sense_object is not a proper powerflow object!",obj->name);
					/*  TROUBLESHOOT
					When in LOAD_FOLLOWING mode, the inverter requires an appropriate connection
					to a powerflow object to sense the current load.  This can be either a meter,
					triplex_meter, or transformer at the current time.
					*/
					return 0;
				}

				//Check lockout times
				if (charge_lockout_time<0)
				{
					gl_error("inverter:%s - charge_lockout_time is negative!",obj->name);
					/*  TROUBLESHOOT
					The charge_lockout_time for the inverter is negative.  Negative lockout times
					are not allowed inside the inverter.  Please correct the value and try again.
					*/
					return 0;
				}
				else if (charge_lockout_time == 0.0)
				{
					gl_warning("inverter:%s - charge_lockout_time is zero, oscillations may occur",obj->name);
					/*  TROUBLESHOOT
					The value for charge_lockout_time is zero, which means there is no delay in new dispatch
					operations.  This may result in excessive switching and iteration limits being hit.  If this is
					not desired, specify a charge_lockout_time larger than zero.
					*/
				}
				//Defaulted else, must be okay

				if (discharge_lockout_time<0)
				{
					gl_error("inverter:%s - discharge_lockout_time is negative!",obj->name);
					/*  TROUBLESHOOT
					The discharge_lockout_time for the inverter is negative.  Negative lockout times
					are not allowed inside the inverter.  Please correct the value and try again.
					*/
					return 0;
				}
				else if (discharge_lockout_time == 0.0)
				{
					gl_warning("inverter:%s - discharge_lockout_time is zero, oscillations may occur",obj->name);
					/*  TROUBLESHOOT
					The value for discharge_lockout_time is zero, which means there is no delay in new dispatch
					operations.  This may result in excessive switching and iteration limits being hit.  If this is
					not desired, specify a discharge_lockout_time larger than zero.
					*/
				}
				//Defaulted else, must be okay
			}//End LOAD_FOLLOWING checks

			if (inv_eta==0)
			{
				efficiency = 0.9;	//Unclear why this is split in 4-quadrant, but not adjusting for fear of breakage
				gl_warning("Efficiency unspecified - defaulted to %f for this inverter type",efficiency);
				//defined above
			}
			if(inv_eta == 0){
				inv_eta = 0.9;
			} else if(inv_eta < 0){
				gl_warning("Inverter efficiency must be positive--using default value");
				inv_eta = 0.9;
			}
			if(p_rated == 0){
				//throw("Inverter must have a nonzero power rating.");
				gl_warning("Inverter must have a nonzero power rating--using default value");
				p_rated = 25000;
			}
			if(number_of_phases_out == 1){
				bp_rated = p_rated/inv_eta;
			} else if(number_of_phases_out == 2){
				bp_rated = 2*p_rated/inv_eta;
			} else if(number_of_phases_out == 3){
				bp_rated = 3*p_rated/inv_eta;
			}
			if(use_multipoint_efficiency == FALSE)
				if(p_max == -1){
					p_max = bp_rated*inv_eta;
				}
			break;
		default:
			//Never supposed to really get here
			GL_THROW("Invalid inverter type specified!");
			/*  TROUBLESHOOT
			An invalid inverter type was specified for the property inverter_type.  Please select one of
			the acceptable types and try again.
			*/
			break;
	}

	//Make sure efficiency is not an invalid value
	if ((efficiency<=0) || (efficiency>1))
	{
		GL_THROW("The efficiency specified for inverter:%s is invalid",obj->name);
		/*  TROUBLESHOOT
		The efficiency value specified must be greater than zero and less than or equal to
		1.0.  Please specify a value in that range.
		*/
	}

	//internal_switch_resistance(switch_type_choice);
	filter_circuit_impact((power_electronics::FILTER_TYPE)filter_type_v, 
		(power_electronics::FILTER_IMPLEMENTATION)filter_imp_v);

	//seting up defaults for multipoint efficiency
	if(use_multipoint_efficiency == TRUE){
		switch(inverter_manufacturer){//all manufacurer defaults use the CEC parameters
			case NONE:
				if(p_dco < 0){
					gl_error("no maximum dc power was given for the inverter.");
					return 0;
				}
				if(v_dco < 0){
					gl_error("no maximum dc voltage was given for the inverter.");
					return 0;
				}
				if(p_so < 0){
					gl_error("no minimum dc power was given for the inverter.");
					return 0;
				}
				if(p_rated <= 0){
					gl_error("no rated per phase power was given for the inverter.");
					return 0;
				} else {
					switch (number_of_phases_out)
					{
						// single phase connection
						case 1:
							p_max = p_rated;
							break;
						// two-phase connection
						case 2:
							p_max = 2*p_rated;
							break;
						// three-phase connection
						case 3:
							p_max = 3*p_rated;
							break;
						default:
							//Never supposed to really get here
							GL_THROW("Invalid phase configuration specified!");
							/*  TROUBLESHOOT
							An invalid phase congifuration was specified when attaching to the "parent" object.  Please report this
							error.
							*/
							break;
					}
				}
				if(c_o == -1){
					c_o = 0;
				}
				if(c_1 == -1){
					c_1 = 0;
				}
				if(c_2 == -1){
					c_2 = 0;
				}
				if(c_3 == -1){
					c_3 = 0;
				}
				break;
			case FRONIUS:
				if(p_dco < 0){
					p_dco = 2879;
				}
				if(v_dco < 0){
					v_dco = 277;
				}
				if(p_so < 0){
					p_so = 27.9;
				}
				if(c_o == -1){
					c_o = -1.009e-5;
				}
				if(c_1 == -1){
					c_1 = -1.367e-5;
				}
				if(c_2 == -1){
					c_2 = -3.587e-5;
				}
				if(c_3 == -1){
					c_3 = -3.421e-3;
				}
				if(p_max < 0){
					p_max = 2700;
					switch (number_of_phases_out)
					{
						// single phase connection
						case 1:
							p_rated = p_max;
							break;
						// two-phase connection
						case 2:
							p_rated = p_max/2;
							break;
						// three-phase connection
						case 3:
							p_rated = p_max/3;
							break;
						default:
							//Never supposed to really get here
							GL_THROW("Invalid phase configuration specified!");
							/*  TROUBLESHOOT
							An invalid phase congifuration was specified when attaching to the "parent" object.  Please report this
							error.
							*/
							break;
					}
				}
				break;
			case SMA:
				if(p_dco < 0){
					p_dco = 2694;
				}
				if(v_dco < 0){
					v_dco = 302;
				}
				if(p_so < 0){
					p_so = 20.7;
				}
				if(c_o == -1){
					c_o = -1.545e-5;
				}
				if(c_1 == -1){
					c_1 = 6.525e-5;
				}
				if(c_2 == -1){
					c_2 = 2.836e-3;
				}
				if(c_3 == -1){
					c_3 = -3.058e-4;
				}
				if(p_max < 0){
					p_max = 2500;
					switch (number_of_phases_out)
					{
						// single phase connection
						case 1:
							p_rated = p_max;
							break;
						// two-phase connection
						case 2:
							p_rated = p_max/2;
							break;
						// three-phase connection
						case 3:
							p_rated = p_max/3;
							break;
						default:
							//Never supposed to really get here
							GL_THROW("Invalid phase configuration specified!");
							/*  TROUBLESHOOT
							An invalid phase congifuration was specified when attaching to the "parent" object.  Please report this
							error.
							*/
							break;
					}
				}
				break;
			case XANTREX:
				if(p_dco < 0){
					p_dco = 4022;
				}
				if(v_dco < 0){
					v_dco = 266;
				}
				if(p_so < 0){
					p_so = 24.1;
				}
				if(c_o == -1){
					c_o = -8.425e-6;
				}
				if(c_1 == -1){
					c_1 = 8.590e-6;
				}
				if(c_2 == -1){
					c_2 = 7.76e-4;
				}
				if(c_3 == -1){
					c_3 = -5.278e-4;
				}
				if(p_max < 0){
					p_max = 3800;
					switch (number_of_phases_out)
					{
						// single phase connection
						case 1:
							p_rated = p_max;
							break;
						// two-phase connection
						case 2:
							p_rated = p_max/2;
							break;
						// three-phase connection
						case 3:
							p_rated = p_max/3;
							break;
						default:
							//Never supposed to really get here
							GL_THROW("Invalid phase configuration specified!");
							/*  TROUBLESHOOT
							An invalid phase congifuration was specified when attaching to the "parent" object.  Please report this
							error.
							*/
							break;
					}
				}
				break;
		}
		if(p_max > p_dco){
			gl_error("The maximum dc power into the inverter cannot be less than the maximum ac power out.");
			return 0;
		}
		if(p_so > p_dco){
			gl_error("The maximum dc power into the inverter cannot be less than the minimum dc power.");
			return 0;
		}
	}

	return 1;
}

TIMESTAMP inverter::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	OBJECT *obj = OBJECTHDR(this);
	if(inverter_type_v != FOUR_QUADRANT){
		phaseA_I_Out = phaseB_I_Out = phaseC_I_Out = 0.0;
	} else {
		if(four_quadrant_control_mode == FQM_LOAD_FOLLOWING)
		{
			if (t1 != t0)
			{
				//See if the "new" timestamp allows us to change
				if (t1>=next_update_time)
				{
					//Above the previous time, so allow a change
					lf_dispatch_change_allowed = true;
				}

				//Threshold checks
				if (max_charge_rate <0)
				{
					GL_THROW("inverter:%s - max_charge_rate is negative!",obj->name);
					/*  TROUBLESHOOT
					The max_charge_rate for the inverter is negative.  Please specify
					a valid charge rate for the object to continue.
					*/
				}
				else if (max_charge_rate == 0)
				{
					gl_warning("inverter:%s - max_charge_rate is zero",obj->name);
					/*  TROUBLESHOOT
					The max_charge_rate for the inverter is currently zero.  This will result
					in no charging action by the inverter.  If this is not desired, please specify a valid
					value.
					*/
				}

				if (max_discharge_rate <0)
				{
					GL_THROW("inverter:%s - max_discharge_rate is negative!",obj->name);
					/*  TROUBLESHOOT
					The max_discharge_rate for the inverter is negative.  Please specify
					a valid discharge rate for the object to continue.
					*/
				}
				else if (max_discharge_rate == 0)
				{
					gl_warning("inverter:%s - max_discharge_rate is zero",obj->name);
					/*  TROUBLESHOOT
					The max_discharge_rate for the inverter is currently zero.  This will result
					in no discharging action by the inverter.  If this is not desired, please specify a valid
					value.
					*/
				}

				//Charge thresholds
				if (charge_on_threshold > charge_off_threshold)
				{
					GL_THROW("inverter:%s - charge_on_threshold is greater than charge_off_threshold!",obj->name);
					/*  TROUBLESHOOT
					For proper LOAD_FOLLOWING behavior, charge_on_threshold should be smaller than charge_off_threshold.
					Please correct this and try again.
					*/
				}
				else if (charge_on_threshold == charge_off_threshold)
				{
					gl_warning("inverter:%s - charge_on_threshold and charge_off_threshold are equal - may not behave properly!",obj->name);
					/*  TROUBLESHOOT
					For proper LOAD_FOLLOWING operation, charge_on_threshold and charge_off_threshold should specify a deadband for operation.
					If equal, the inverter may not operate properly and the system may never solve properly.
					*/
				}

				//Discharge thresholds
				if (discharge_on_threshold < discharge_off_threshold)
				{
					GL_THROW("inverter:%s - discharge_on_threshold is less than discharge_off_threshold!",obj->name);
					/*  TROUBLESHOOT
					For proper LOAD_FOLLOWING behavior, discharge_on_threshold should be larger than discharge_off_threshold.
					Please correct this and try again.
					*/
				}
				else if (discharge_on_threshold == discharge_off_threshold)
				{
					gl_warning("inverter:%s - discharge_on_threshold and discharge_off_threshold are equal - may not behave properly!",obj->name);
					/*  TROUBLESHOOT
					For proper LOAD_FOLLOWING operation, discharge_on_threshold and discharge_off_threshold should specify a deadband for operation.
					If equal, the inverter may not operate properly and the system may never solve properly.
					*/
				}

				//Combination of the two
				if (discharge_off_threshold <= charge_off_threshold)
				{
					gl_warning("inverter:%s - discharge_off_threshold should be larger than the charge_off_threshold",obj->name);
					/*  TROUBLESHOOT
					For proper LOAD_FOLLOWING operation, the deadband for the inverter should not overlap.  Please specify a larger
					range for the discharge and charge bands of operation and try again.  If the bands do overlap, unexpected behavior may occur.
					*/
				}
			}
		}//End LOAD_FOLLOWING checks
	}
		
	return t2; 
}

TIMESTAMP inverter::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);

	if (*pMeterStatus==1)	//Make sure the meter is in service
	{
		phaseA_V_Out = pCircuit_V[0];	//Syncs the meter parent to the generator.
		phaseB_V_Out = pCircuit_V[1];
		phaseC_V_Out = pCircuit_V[2];

		internal_losses = 1 - calculate_loss(Rtotal, Ltotal, Ctotal, DC, AC);
		frequency_losses = 1 - calculate_frequency_loss(output_frequency, Rtotal,Ltotal, Ctotal);

		if(inverter_type_v != FOUR_QUADRANT)
		{
			switch(gen_mode_v)
			{
				case CONSTANT_PF:
					VA_In = V_In * ~ I_In; //DC

					// need to differentiate between different pulses...
					if(use_multipoint_efficiency == FALSE){
						VA_Out = VA_In * efficiency * internal_losses * frequency_losses;
					} else {
						if(VA_In <= p_so){
							VA_Out = 0;
						} else {
							if(V_In > v_dco){
								gl_warning("The dc voltage is greater than the specified maximum for the inverter. Efficiency model may be inaccurate.");
							}
							C1 = p_dco*(1+c_1*(V_In.Re()-v_dco));
							C2 = p_so*(1+c_2*(V_In.Re()-v_dco));
							C3 = c_o*(1+c_3*(V_In.Re()-v_dco));
							VA_Out.SetReal((((p_max/(C1-C2))-C3*(C1-C2))*(VA_In.Re()-C2)+C3*(VA_In.Re()-C2)*(VA_In.Re()-C2))*internal_losses*frequency_losses);
						}
					}
					//losses = VA_Out * Rtotal / (Rtotal + Rload);
					//VA_Out = VA_Out * Rload / (Rtotal + Rload);

					if ((phases & 0x10) == 0x10)  //Triplex-line -> Assume it's only across the 240 V for now.
					{
						power_A = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)));
						if (phaseA_V_Out.Mag() != 0.0)
							phaseA_I_Out = ~(power_A / phaseA_V_Out);
						else
							phaseA_I_Out = complex(0.0,0.0);

						*pLine12 += -phaseA_I_Out;

						//Update this value for later removal
						last_current[3] = -phaseA_I_Out;
						
						//Get rid of these for now
						//complex phaseA_V_Internal = filter_voltage_impact_source(phaseA_I_Out, phaseA_V_Out);
						//phaseA_I_Out = filter_current_impact_out(phaseA_I_Out, phaseA_V_Internal);
					}
					else if (number_of_phases_out == 3) // All three phases
					{
						power_A = power_B = power_C = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)))/3;
						if (phaseA_V_Out.Mag() != 0.0)
							phaseA_I_Out = ~(power_A / phaseA_V_Out); // /sqrt(2.0);
						else
							phaseA_I_Out = complex(0.0,0.0);
						if (phaseB_V_Out.Mag() != 0.0)
							phaseB_I_Out = ~(power_B / phaseB_V_Out); // /sqrt(2.0);
						else
							phaseB_I_Out = complex(0.0,0.0);
						if (phaseC_V_Out.Mag() != 0.0)
							phaseC_I_Out = ~(power_C / phaseC_V_Out); // /sqrt(2.0);
						else
							phaseC_I_Out = complex(0.0,0.0);

						pLine_I[0] += -phaseA_I_Out;
						pLine_I[1] += -phaseB_I_Out;
						pLine_I[2] += -phaseC_I_Out;

						//Update this value for later removal
						last_current[0] = -phaseA_I_Out;
						last_current[1] = -phaseB_I_Out;
						last_current[2] = -phaseC_I_Out;

						//complex phaseA_V_Internal = filter_voltage_impact_source(phaseA_I_Out, phaseA_V_Out);
						//complex phaseB_V_Internal = filter_voltage_impact_source(phaseB_I_Out, phaseB_V_Out);
						//complex phaseC_V_Internal = filter_voltage_impact_source(phaseC_I_Out, phaseC_V_Out);

						//phaseA_I_Out = filter_current_impact_out(phaseA_I_Out, phaseA_V_Internal);
						//phaseB_I_Out = filter_current_impact_out(phaseB_I_Out, phaseB_V_Internal);
						//phaseC_I_Out = filter_current_impact_out(phaseC_I_Out, phaseC_V_Internal);
					}
					else if(number_of_phases_out == 2) // two-phase connection
					{
						OBJECT *obj = OBJECTHDR(this);

						if ( ((phases & 0x01) == 0x01) && phaseA_V_Out.Mag() != 0)
						{
							power_A = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)))/2;;
							phaseA_I_Out = ~(power_A / phaseA_V_Out);
						}
						else 
							phaseA_I_Out = complex(0,0);

						if ( ((phases & 0x02) == 0x02) && phaseB_V_Out.Mag() != 0)
						{
							power_B = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)))/2;;
							phaseB_I_Out = ~(power_B / phaseB_V_Out);
						}
						else 
							phaseB_I_Out = complex(0,0);

						if ( ((phases & 0x04) == 0x04) && phaseC_V_Out.Mag() != 0)
						{
							power_C = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)))/2;;
							phaseC_I_Out = ~(power_C / phaseC_V_Out);
						}
						else 
							phaseC_I_Out = complex(0,0);

						pLine_I[0] += -phaseA_I_Out;
						pLine_I[1] += -phaseB_I_Out;
						pLine_I[2] += -phaseC_I_Out;

						//Update this value for later removal
						last_current[0] = -phaseA_I_Out;
						last_current[1] = -phaseB_I_Out;
						last_current[2] = -phaseC_I_Out;

					}
					else // Single phase connection
					{
						if( ((phases & 0x01) == 0x01) && phaseA_V_Out.Mag() != 0)
						{
							power_A = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)));
							phaseA_I_Out = ~(power_A / phaseA_V_Out); 
							//complex phaseA_V_Internal = filter_voltage_impact_source(phaseA_I_Out, phaseA_V_Out);
							//phaseA_I_Out = filter_current_impact_out(phaseA_I_Out, phaseA_V_Internal);
						}
						else if( ((phases & 0x02) == 0x02) && phaseB_V_Out.Mag() != 0)
						{
							power_B = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)));
							phaseB_I_Out = ~(power_B / phaseB_V_Out); 
							//complex phaseB_V_Internal = filter_voltage_impact_source(phaseB_I_Out, phaseB_V_Out);
							//phaseB_I_Out = filter_current_impact_out(phaseB_I_Out, phaseB_V_Internal);
						}
						else if( ((phases & 0x04) == 0x04) && phaseC_V_Out.Mag() != 0)
						{
							power_C = complex(VA_Out.Mag()*fabs(power_factor),power_factor/fabs(power_factor)*VA_Out.Mag()*sin(acos(power_factor)));
							phaseC_I_Out = ~(power_C / phaseC_V_Out); 
							//complex phaseC_V_Internal = filter_voltage_impact_source(phaseC_I_Out, phaseC_V_Out);
							//phaseC_I_Out = filter_current_impact_out(phaseC_I_Out, phaseC_V_Internal);
						}
						else
						{
							gl_warning("None of the phases specified have voltages!");
							phaseA_I_Out = phaseB_I_Out = phaseC_I_Out = complex(0.0,0.0);
						}
						pLine_I[0] += -phaseA_I_Out;
						pLine_I[1] += -phaseB_I_Out;
						pLine_I[2] += -phaseC_I_Out;

						//Update this value for later removal
						last_current[0] = -phaseA_I_Out;
						last_current[1] = -phaseB_I_Out;
						last_current[2] = -phaseC_I_Out;

					}
					return TS_NEVER;
					break;
				case CONSTANT_PQ:
					GL_THROW("Constant PQ mode not supported at this time");
					/* TROUBLESHOOT
					This will be worked on at a later date and is not yet correctly implemented.
					*/
					gl_verbose("inverter sync: constant pq");
					//TODO
					//gather V_Out for each phase
					//gather V_In (DC) from line -- can not gather V_In, for now set equal to V_Out
					//P_Out is either set or input from elsewhere
					//Q_Out is either set or input from elsewhere
					//Gather Rload

					if(parent_string == "meter")
					{
						VA_Out = complex(P_Out,Q_Out);
					}
					else if (parent_string == "triplex_meter")
					{
						VA_Out = complex(P_Out,Q_Out);
					}
					else
					{
						phaseA_I_Out = pLine_I[0];
						phaseB_I_Out = pLine_I[1];
						phaseC_I_Out = pLine_I[2];

						//Erm, there's no good way to handle this from a "multiply attached" point of view.
						//TODO: Think about how to do this if the need arrises

						VA_Out = phaseA_V_Out * (~ phaseA_I_Out) + phaseB_V_Out * (~ phaseB_I_Out) + phaseC_V_Out * (~ phaseC_I_Out);
					}

					pf_out = P_Out/VA_Out.Mag();
					
					//VA_Out = VA_In * efficiency * internal_losses;

					if ( (phases & 0x07) == 0x07) // Three phase
					{
						power_A = power_B = power_C = VA_Out /3;
						phaseA_I_Out = (power_A / phaseA_V_Out); // /sqrt(2.0);
						phaseB_I_Out = (power_B / phaseB_V_Out); // /sqrt(2.0);
						phaseC_I_Out = (power_C / phaseC_V_Out); // /sqrt(2.0);

						phaseA_I_Out = ~ phaseA_I_Out;
						phaseB_I_Out = ~ phaseB_I_Out;
						phaseC_I_Out = ~ phaseC_I_Out;

					}
					else if ( (number_of_phases_out == 1) && ((phases & 0x01) == 0x01) ) // Phase A only
					{
						power_A = VA_Out;
						phaseA_I_Out = (power_A / phaseA_V_Out); // /sqrt(2);
						phaseA_I_Out = ~ phaseA_I_Out;
					}
					else if ( (number_of_phases_out == 1) && ((phases & 0x02) == 0x02) ) // Phase B only
					{
						power_B = VA_Out;
						phaseB_I_Out = (power_B / phaseB_V_Out);  // /sqrt(2);
						phaseB_I_Out = ~ phaseB_I_Out;
					}
					else if ( (number_of_phases_out == 1) && ((phases & 0x04) == 0x04) ) // Phase C only
					{
						power_C = VA_Out;
						phaseC_I_Out = (power_C / phaseC_V_Out); // /sqrt(2);
						phaseC_I_Out = ~ phaseC_I_Out;
					}
					else
					{
						throw ("unsupported number of phases");
					}

					VA_In = VA_Out / (efficiency * internal_losses * frequency_losses);
					losses = VA_Out * (1 - (efficiency * internal_losses * frequency_losses));

					//V_In = complex(0,0);
					//
					////is there a better way to do this?
					//if(phaseAOut){
					//	V_In += abs(phaseA_V_Out.Re());
					//}
					//if(phaseBOut){
					//	V_In += abs(phaseB_V_Out.Re());
					//}
					//if(phaseCOut){
					//	V_In += abs(phaseC_V_Out.Re());
					//}else{
					//	throw ("none of the phases have voltages!");
					//}

					V_In = Vdc;



					I_In = VA_In / V_In;
					I_In = ~I_In;

					V_In = filter_voltage_impact_source(I_In, V_In);
					I_In = filter_current_impact_source(I_In, V_In);

					gl_verbose("Inverter sync: V_In asked for by inverter is: (%f , %f)", V_In.Re(), V_In.Im());
					gl_verbose("Inverter sync: I_In asked for by inverter is: (%f , %f)", I_In.Re(), I_In.Im());


					pLine_I[0] += phaseA_I_Out;
					pLine_I[1] += phaseB_I_Out;
					pLine_I[2] += phaseC_I_Out;

					//Update this value for later removal
					last_current[0] = phaseA_I_Out;
					last_current[1] = phaseB_I_Out;
					last_current[2] = phaseC_I_Out;

					return TS_NEVER;
					break;
				case CONSTANT_V:
				{
					GL_THROW("Constant V mode not supported at this time");
					/* TROUBLESHOOT
					This will be worked on at a later date and is not yet correctly implemented.
					*/
					gl_verbose("inverter sync: constant v");
					bool changed = false;
					
					//TODO
					//Gather V_Out
					//Gather VA_Out
					//Gather Rload
					if(phaseAOut)
					{
						if (phaseA_V_Out.Re() < (V_Set_A - margin))
						{
							phaseA_I_Out = phaseA_I_Out_prev + I_step_max/2;
							changed = true;
						}
						else if (phaseA_V_Out.Re() > (V_Set_A + margin))
						{
							phaseA_I_Out = phaseA_I_Out_prev - I_step_max/2;
							changed = true;
						}
						else
						{
							changed = false;
						}
					}
					if (phaseBOut)
					{
						if (phaseB_V_Out.Re() < (V_Set_B - margin))
						{
							phaseB_I_Out = phaseB_I_Out_prev + I_step_max/2;
							changed = true;
						}
						else if (phaseB_V_Out.Re() > (V_Set_B + margin))
						{
							phaseB_I_Out = phaseB_I_Out_prev - I_step_max/2;
							changed = true;
						}
						else
						{
							changed = false;
						}
					}
					if (phaseCOut)
					{
						if (phaseC_V_Out.Re() < (V_Set_C - margin))
						{
							phaseC_I_Out = phaseC_I_Out_prev + I_step_max/2;
							changed = true;
						}
						else if (phaseC_V_Out.Re() > (V_Set_C + margin))
						{
							phaseC_I_Out = phaseC_I_Out_prev - I_step_max/2;
							changed = true;
						}
						else
						{
							changed = false;
						}
					}
					
					power_A = (~phaseA_I_Out) * phaseA_V_Out;
					power_B = (~phaseB_I_Out) * phaseB_V_Out;
					power_C = (~phaseC_I_Out) * phaseC_V_Out;

					//check if inverter is overloaded -- if so, cap at max power
					if (((power_A + power_B + power_C) > Rated_kVA) ||
						((power_A.Re() + power_B.Re() + power_C.Re()) > Max_P) ||
						((power_A.Im() + power_B.Im() + power_C.Im()) > Max_Q))
					{
						VA_Out = Rated_kVA / number_of_phases_out;
						//if it's maxed out, don't ask for the simulator to re-call
						changed = false;
						if(phaseAOut)
						{
							phaseA_I_Out = VA_Out / phaseA_V_Out;
							phaseA_I_Out = (~phaseA_I_Out);
						}
						if(phaseBOut)
						{
							phaseB_I_Out = VA_Out / phaseB_V_Out;
							phaseB_I_Out = (~phaseB_I_Out);
						}
						if(phaseCOut)
						{
							phaseC_I_Out = VA_Out / phaseC_V_Out;
							phaseC_I_Out = (~phaseC_I_Out);
						}
					}
					
					//check if power is negative for some reason, should never be
					if(power_A < 0)
					{
						power_A = 0;
						phaseA_I_Out = 0;
						throw("phaseA power is negative!");
					}
					if(power_B < 0)
					{
						power_B = 0;
						phaseB_I_Out = 0;
						throw("phaseB power is negative!");
					}
					if(power_C < 0)
					{
						power_C = 0;
						phaseC_I_Out = 0;
						throw("phaseC power is negative!");
					}

					VA_In = VA_Out / (efficiency * internal_losses * frequency_losses);
					losses = VA_Out * (1 - (efficiency * internal_losses * frequency_losses));

					//V_In = complex(0,0);
					//
					////is there a better way to do this?
					//if(phaseAOut){
					//	V_In += abs(phaseA_V_Out.Re());
					//}
					//if(phaseBOut){
					//	V_In += abs(phaseB_V_Out.Re());
					//}
					//if(phaseCOut){
					//	V_In += abs(phaseC_V_Out.Re());
					//}else{
					//	throw ("none of the phases have voltages!");
					//}

					V_In = Vdc;

					I_In = VA_In / V_In;
					I_In  = ~I_In;
					
					gl_verbose("Inverter sync: I_In asked for by inverter is: (%f , %f)", I_In.Re(), I_In.Im());

					V_In = filter_voltage_impact_source(I_In, V_In);
					I_In = filter_current_impact_source(I_In, V_In);

					//TODO: check P and Q components to see if within bounds

					if(changed)
					{
						pLine_I[0] += phaseA_I_Out;
						pLine_I[1] += phaseB_I_Out;
						pLine_I[2] += phaseC_I_Out;
						
						//Update this value for later removal
						last_current[0] = phaseA_I_Out;
						last_current[1] = phaseB_I_Out;
						last_current[2] = phaseC_I_Out;

						TIMESTAMP t2 = t1 + 10 * 60 * TS_SECOND;
						return t2;
					}
					else
					{
						pLine_I[0] += phaseA_I_Out;
						pLine_I[1] += phaseB_I_Out;
						pLine_I[2] += phaseC_I_Out;

						//Update this value for later removal
						last_current[0] = phaseA_I_Out;
						last_current[1] = phaseB_I_Out;
						last_current[2] = phaseC_I_Out;

						return TS_NEVER;
					}
					break;
				}
				case SUPPLY_DRIVEN: 
					GL_THROW("SUPPLY_DRIVEN mode for inverters not supported at this time");
					break;
				default:
					pLine_I[0] += phaseA_I_Out;
					pLine_I[1] += phaseB_I_Out;
					pLine_I[2] += phaseC_I_Out;

					//Update this value for later removal
					last_current[0] = phaseA_I_Out;
					last_current[1] = phaseB_I_Out;
					last_current[2] = phaseC_I_Out;

					return TS_NEVER;
					break;
			}

			if ( (phases & 0x10) == 0x10 ) // split phase
			{
				*pLine12 += phaseA_I_Out;

				//Update this value for later removal
				last_current[3] = phaseA_I_Out;
			}
			else
			{
				pLine_I[0] += phaseA_I_Out;
				pLine_I[1] += phaseB_I_Out;
				pLine_I[2] += phaseC_I_Out;

				//Update this value for later removal
				last_current[0] = phaseA_I_Out;
				last_current[1] = phaseB_I_Out;
				last_current[2] = phaseC_I_Out;
			}
		} 
		else	//FOUR_QUADRANT code
		{
			//FOUR_QUADRANT model (originally written for NAS/CES, altered for PV)
			double VA_Efficiency, temp_PF, temp_QVal;
			complex temp_VA;
			complex battery_power_out = complex(0,0);
			
			//Compute power in - supposedly DC, but since it's complex, we'll be proper (other models may need fixing)
			VA_In = V_In * ~ I_In;

			//Compute the power contribution of the battery object
			if((phases & 0x10) == 0x10){ // split phase
				battery_power_out = power_A;
			} else { // three phase
				if((phases & 0x01) == 0x01){ // has phase A
					battery_power_out += power_A;
				}
				if((phases & 0x02) == 0x02){ // hase phase B
					battery_power_out += power_B;
				}
				if((phases & 0x04) == 0x04){ // has phase C
					battery_power_out += power_C;
				}
			}
			//Determine how to efficiency weight it
			if(use_multipoint_efficiency == false)
			{
				//Normal scaling
				VA_Efficiency = VA_In.Re() * efficiency * internal_losses * frequency_losses;
			}
			else
			{
				//See if above minimum DC power input
				if(VA_In.Mag() <= p_so)
				{
					VA_Efficiency = 0.0;	//Nope, no output
				}
				else	//Yes, apply effiency change
				{
					//Make sure voltage isn't too low
					if(V_In.Mag() > v_dco)
					{
						gl_warning("The dc voltage is greater than the specified maximum for the inverter. Efficiency model may be inaccurate.");
						/*  TROUBLESHOOT
						The DC voltage at the input to the inverter is less than the maximum voltage supported by the inverter.  As a result, the
						multipoint efficiency model may not provide a proper result.
						*/
					}

					//Compute coefficients for multipoint efficiency
					C1 = p_dco*(1+c_1*(V_In.Re()-v_dco));
					C2 = p_so*(1+c_2*(V_In.Re()-v_dco));
					C3 = c_o*(1+c_3*(V_In.Re()-v_dco));

					//Apply this to the output
					VA_Efficiency = (((p_max/(C1-C2))-C3*(C1-C2))*(VA_In.Re()-C2)+C3*(VA_In.Re()-C2)*(VA_In.Re()-C2))*internal_losses*frequency_losses;
				}
			}
			VA_Efficiency += battery_power_out.Mag();
			//Determine 4 quadrant outputs
			if(four_quadrant_control_mode == FQM_CONSTANT_PF)	//Power factor mode
			{
				if(power_factor != 0.0)	//Not purely imaginary
				{
					if (VA_In<0.0)	//Discharge at input, so must be "load"
					{
						//Total power output is the magnitude
						VA_Out.SetReal(VA_Efficiency*fabs(power_factor));
					}
					else	//Positive input, so must be generator
					{
						//Total power output is the magnitude
						VA_Out.SetReal(VA_Efficiency*fabs(power_factor)*-1.0);
					}

					//Apply power factor sign properly - + sign is lagging in, which is proper here
					//Treat like a normal load right now
					if (power_factor < 0)
					{
						VA_Out.SetImag(VA_Efficiency*-1.0*sqrt(1.0-(power_factor*power_factor)));
					}
					else	//Must be positive
					{
						VA_Out.SetImag(VA_Efficiency*sqrt(1.0-(power_factor*power_factor)));
					}
				}
				else	//Purely imaginary value
				{
					VA_Out = complex(0.0,VA_Efficiency);
				}
			}
			else if (four_quadrant_control_mode == FQM_CONSTANT_PQ)
			{
				//Compute desired output - sign convention appears to be backwards
				temp_VA = complex(-P_Out,-Q_Out);

				//See if we exceed input power
				if (temp_VA.Mag() > VA_Efficiency)
				{
					//Over-sized - previous implementation gave preference to real power
					if (VA_Efficiency > fabs(temp_VA.Re()))	//More "available" power than input
					{
						//Determine the Q we can provide
						temp_QVal = sqrt((VA_Efficiency*VA_Efficiency) - (temp_VA.Re()*temp_VA.Re()));

						//Assign to output, negating signs as necessary (temp_VA already negated)
						if (temp_VA.Im() < 0.0)	//Negative Q dispatch
						{
							VA_Out = complex(temp_VA.Re(),-temp_QVal);
						}
						else	//Positive Q dispatch
						{
							VA_Out = complex(temp_VA.Re(),temp_QVal);
						}
					}
					else	//Equal to or smaller than real power desired, give it all we go
					{
						//Maintain desired sign convention
						if (temp_VA.Re() < 0.0)
						{
							VA_Out = complex(-VA_Efficiency,0.0);
						}
						else	//Positive
						{
							VA_Out = complex(VA_Efficiency,0.0);
						}
					}
				}
				else	//Doesn't exceed, assign it
				{
					//Inverter rating clipping will occur below
					VA_Out = temp_VA;
				}
			}
			else if (four_quadrant_control_mode == FQM_LOAD_FOLLOWING)
			{
				VA_Out = lf_dispatch_power;	//Place the expected dispatch power into the output
			}
			//Not implemented and removed from above, so no check needed
			//else if(four_quadrant_control_mode == FQM_CONSTANT_V){
			//	GL_THROW("CONSTANT_V mode is not supported at this time.");
			//} else if(four_quadrant_control_mode == FQM_VOLT_VAR){
			//	GL_THROW("VOLT_VAR mode is not supported at this time.");
			//}
			
			//check to see if VA_Out is within rated absolute power rating
			if(VA_Out.Mag() > p_max)
			{
				//Determine the excess, for use elsewhere - back out simple efficiencies
				excess_input_power = (VA_Out.Mag() - p_max)/(internal_losses*frequency_losses);

				//Apply thresholding - going on the assumption of maintaining vector direction
				if (four_quadrant_control_mode == FQM_CONSTANT_PF)
				{
					temp_PF = power_factor;
				}
				else	//Extract it - overall value (signs handled separately)
				{
					temp_PF = VA_Out.Re()/VA_Out.Mag();
				}

				//Compute the "new" output - signs lost
				temp_VA = complex(fabs(p_max*temp_PF),fabs(p_max*sqrt(1.0-(temp_PF*temp_PF))));

				//"Sign" it appropriately
				if ((VA_Out.Re()<0) && (VA_Out.Im()<0))	//-R, -I
				{
					VA_Out = -temp_VA;
				}
				else if ((VA_Out.Re()<0) && (VA_Out.Im()>=0))	//-R,I
				{
					VA_Out = complex(-temp_VA.Re(),temp_VA.Im());
				}
				else if ((VA_Out.Re()>=0) && (VA_Out.Im()<0))	//R,-I
				{
					VA_Out = complex(temp_VA.Re(),-temp_VA.Im());
				}
				else	//R,I
				{
					VA_Out = temp_VA;
				}
			}
			else	//Not over, zero "overrage"
			{
				excess_input_power = 0.0;
			}

			//See if load following, if so, make sure storage is appropriate - only considers real right now
			if (four_quadrant_control_mode == FQM_LOAD_FOLLOWING || battery_power_out.Mag() != 0.0)
			{
				if ((b_soc == 1.0) && (VA_Out.Re() > 0))	//Battery full and positive influx of real power
				{
					gl_warning("inverter:%s - battery full - no charging allowed",obj->name);
					/*  TROUBLESHOOT
					In LOAD_FOLLOWING mode, a full battery status was encountered.  The inverter is unable
					to sink any further energy, so consumption was set to zero.
					*/
					VA_Out.SetReal(0.0);	//Set to zero - reactive considerations may change this
				}
				else if ((b_soc <= soc_reserve) && (VA_Out.Re() < 0))	//Battery "empty" and attempting to extract real power
				{
					gl_warning("inverter:%s - battery at or below the SOC reserve - no discharging allowed",obj->name);
					/*  TROUBLESHOOT
					In LOAD_FOLLOWING mode, a empty or "in the SOC reserve margin" battery was encountered and attempted
					to discharge.  The inverter is unable to extract any further power, so the output is set to zero.
					*/
					VA_Out.SetReal(0.0);	//Set output to zero - again, reactive considerations may change this
				}

				//Update values to represent what is being pulled (battery uses for SOC updates) - assumes only storage
				//p_in used by battery - appears reversed to VA_Out
				if (VA_Out.Re() < 0.0)	//Discharging
				{
					p_in = VA_Out.Re()/inv_eta;
				}
				else if (VA_Out.Re() == 0.0)	//Idle
				{
					p_in = 0.0;
				}
				else	//Must be positive, so charging
				{
					p_in = VA_Out.Re()*inv_eta;
				}
			}//End load following battery considerations

			//Assign secondary outputs
			if(four_quadrant_control_mode != FQM_CONSTANT_PQ){
				P_Out = VA_Out.Re();
				Q_Out = VA_Out.Im();
			}

			//Calculate power and post it
			if ((phases & 0x10) == 0x10) // split phase
			{
				//Update last_power variable
				last_power[3] = VA_Out;

				//Post the value
				*pPower +=last_power[3];
			}
			else	//Three phase variant
			{
				//Figure out amount that needs to be posted
				temp_VA = VA_Out/number_of_phases_out;

				if ( (phases & 0x01) == 0x01 ) // has phase A
				{
					last_power[0] = temp_VA;	//Store last power
					pPower[0] += temp_VA;		//Post the current value
				}
				
				if ( (phases & 0x02) == 0x02 ) // has phase B
				{
					last_power[1] = temp_VA;	//Store last power
					pPower[1] += temp_VA;		//Post current value
				}
				
				if ( (phases & 0x04) == 0x04 ) // has phase C
				{
					last_power[2] = temp_VA;	//Store last power
					pPower[2] += temp_VA;		//Post current value
				}
			}//End three-phase variant

			//Negate VA_Out, so it matches sign ideals
			VA_Out = -VA_Out;

		}//End FOUR_QUADRANT mode
		return TS_NEVER;
	}
	else
	{
		//Check to see if we're accumulating or out of service
		if (*pMeterStatus==1)
		{
			if (inverter_type_v != FOUR_QUADRANT)
			{
				//Will only get here on true NR_cycle, if meter is in service
				if ((phases & 0x10) == 0x10)
				{
					*pLine12 += last_current[3];
				}
				else
				{
					pLine_I[0] += last_current[0];
					pLine_I[1] += last_current[1];
					pLine_I[2] += last_current[2];
				}
			}
			else	//Four-quadrant post as power
			{
				//Will only get here on true NR_cycle, if meter is in service
				if ((phases & 0x10) == 0x10)	//Triplex
				{
					*pPower += last_power[3];	//Theoretically pPower is mapped to power_12, which already has the [2] offset applied
				}
				else
				{
					pPower[0] += last_power[0];
					pPower[1] += last_power[1];
					pPower[2] += last_power[2];
				}
			}
		}
		else
		{
			//No contributions, but zero the last_current, just to be safe
			last_current[0] = 0.0;
			last_current[1] = 0.0;
			last_current[2] = 0.0;
			last_current[3] = 0.0;

			//Do the same for power, just for paranoia's sake
			last_power[0] = 0.0;
			last_power[1] = 0.0;
			last_power[2] = 0.0;
			last_power[3] = 0.0;
		}
		return TS_NEVER;
	}
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP inverter::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2 = TS_NEVER;		//By default, we're done forever!
	LOAD_FOLLOW_STATUS new_lf_status;
	double new_lf_dispatch_power, curr_power_val, diff_power_val;				

	//Check and see if we need to redispatch
	if ((inverter_type_v == FOUR_QUADRANT) && (four_quadrant_control_mode == FQM_LOAD_FOLLOWING) && (lf_dispatch_change_allowed==true))
	{
		//See what the sense_object is and determine if we need to update
		if (sense_is_link)
		{
			//Perform the power update
			((void (*)(OBJECT *))(*powerCalc))(sense_object);
		}//End link update of power

		//Extract power, mainly just for convenience, but also to offset us for what we are currently "ordering"
		curr_power_val = sense_power->Re();

		//Check power - only focus on real for now -- this will need to be changed if reactive considered
		if (curr_power_val<=charge_on_threshold)	//Below charging threshold, desire charging
		{
			//See if the battery has room
			if (b_soc < 1.0)
			{
				//See if we were already railed - if less, still have room, or we were discharging (either way okay - just may iterate a couple times)
				if (lf_dispatch_power < max_charge_rate)
				{
					//Make sure we weren't discharging
					if (load_follow_status == DISCHARGE)
					{
						//Set us to idle (will force a reiteration) - who knows where we really are
						new_lf_status = IDLE;
						new_lf_dispatch_power = 0.0;
					}
					else
					{
						//Set us up to charge
						new_lf_status = CHARGE;

						//Determine how far off we are from the "on" point (on is our "maintain" point)
						diff_power_val = charge_on_threshold - curr_power_val;

						//Accumulate it
						new_lf_dispatch_power = lf_dispatch_power + diff_power_val;

						//Make sure it isn't too big
						if (new_lf_dispatch_power > max_charge_rate)
						{
							new_lf_dispatch_power = max_charge_rate;	//Desire more than we can give, limit it
						}
					}
				}
				else	//We were railed, continue with this course
				{
					new_lf_status = CHARGE;						//Keep us charging
					new_lf_dispatch_power = max_charge_rate;	//Set to maximum rate
				}

			}//End Battery has room
			else	//Battery full, no charging allowed
			{
				gl_verbose("inverter:%s - charge desired, but battery full!",obj->name);
				/*  TROUBLESHOOT
				An inverter in LOAD_FOLLOWING mode currently wants to charge the battery more, but the battery
				is full.  Consider using a larger battery and trying again.
				*/

				new_lf_status = IDLE;			//Can't do anything, so we're idle
				new_lf_dispatch_power = 0.0;	//Turn us off
			}
		}//End below charge_on_threshold
		else if (curr_power_val<=charge_off_threshold)	//Below charging off threshold - see what we were doing
		{
			if (load_follow_status != CHARGE)
			{
				new_lf_status = IDLE;			//We were either idle or discharging, so we are now idle
				new_lf_dispatch_power = 0.0;	//Idle power as well
			}
			else	//Must be charging
			{
				//See if the battery has room
				if (b_soc < 1.0)
				{
					new_lf_status = CHARGE;	//Keep us charging

					//Inside this range, just maintain what we were doing
					new_lf_dispatch_power = lf_dispatch_power;
				}//End Battery has room
				else	//Battery full, no charging allowed
				{
					gl_verbose("inverter:%s - charge desired, but battery full!",obj->name);
					//Defined above

					new_lf_status = IDLE;			//Can't do anything, so we're idle
					new_lf_dispatch_power = 0.0;	//Turn us off
				}
			}//End must be charging
		}//End below charge_off_threshold (but above charge_on_threshold)
		else if (curr_power_val<=discharge_off_threshold)	//Below the discharge off threshold - we're idle no matter what here
		{
			new_lf_status = IDLE;			//Nothing occuring, between bands
			new_lf_dispatch_power = 0.0;	//Nothing needed, as a result
		}//End below discharge_off_threshold (but above charge_off_threshold)
		else if (curr_power_val<=discharge_on_threshold)	//Below discharge on threshold - see what we were doing
		{
			//See if we were discharging
			if (load_follow_status == DISCHARGE)
			{
				//Were discharging - see if we can continue
				if (b_soc > soc_reserve)
				{
					new_lf_status = DISCHARGE;	//Keep us discharging

					new_lf_dispatch_power = lf_dispatch_power;	//Maintain what we were doing inside the deadband
				}
				else	//At or below reserve, go to idle
				{
					gl_verbose("inverter:%s - discharge desired, but not enough battery capacity!",obj->name);
					/*  TROUBLESHOOT
					An inverter in LOAD_FOLLOWING mode currently wants to discharge the battery more, but the battery
					is at or below the SOC reserve margin.  Consider using a larger battery and trying again.
					*/

					new_lf_status = IDLE;			//Can't do anything, so we're idle
					new_lf_dispatch_power = 0.0;	//Turn us off
				}
			}
			else	//Must not have been discharging, go idle first since the lack of charge may drop us enough
			{
				new_lf_status = IDLE;			//We'll force a reiteration, maybe
				new_lf_dispatch_power = 0.0;	//Power to match the idle status
			}
		}//End below discharge_on_threshold (but above discharge_off_threshold)
		else	//We're in the discharge realm
		{
			new_lf_status = DISCHARGE;		//Above threshold, so discharge into the grid

			//Check battery status
			if (b_soc > soc_reserve)
			{
				//See if we were charging
				if (load_follow_status == CHARGE)	//were charging, just turn us off and reiterate
				{
					new_lf_status = IDLE;			//Turn us off first, then re-evaluate
					new_lf_dispatch_power = 0.0;	//Represents the idle
				}
				else
				{
					new_lf_status = DISCHARGE;	//Keep us discharging

					//See if we were already railed - if less, still have room
					if (lf_dispatch_power > -max_discharge_rate)
					{
						//Determine how far off we are from the "on" point
						diff_power_val = discharge_on_threshold - curr_power_val;

						//Accumulate it
						new_lf_dispatch_power = lf_dispatch_power + diff_power_val;

						//Make sure it isn't too big
						if (new_lf_dispatch_power < -max_discharge_rate)
						{
							new_lf_dispatch_power = -max_discharge_rate;	//Desire more than we can give, limit it
						}
					}
					else	//We were railed, continue with this course
					{
						new_lf_dispatch_power = -max_discharge_rate;	//Set to maximum discharge rate
					}
				}
			}
			else	//At or below reserve, go to idle
			{
				gl_verbose("inverter:%s - discharge desired, but not enough battery capacity!",obj->name);
				//Defined above

				new_lf_status = IDLE;			//Can't do anything, so we're idle
				new_lf_dispatch_power = 0.0;	//Turn us off
			}
		}//End above discharge_on_threshold

		//Change checks - see if we need to reiterate
		if (new_lf_status != load_follow_status)
		{
			//Update status
			load_follow_status = new_lf_status;

			//Update dispatch, since it obviously changed
			lf_dispatch_power = new_lf_dispatch_power;

			//Major change, force a reiteration
			t2 = t1;

			//Update lockout allowed
			if (new_lf_status == CHARGE)
			{
				//Hard lockout for now
				lf_dispatch_change_allowed = false;

				//Apply update time - note that this may have issues with TS_NEVER, but unsure if it will be a problem at point (theoretically, the simulation is about to end)
				next_update_time = t1 + ((TIMESTAMP)(charge_lockout_time));
			}
			else if (new_lf_status == DISCHARGE)
			{
				//Hard lockout for now
				lf_dispatch_change_allowed = false;

				//Apply update time - note that this may have issues with TS_NEVER, but unsure if it will be a problem at point (theoretically, the simulation is about to end)
				next_update_time = t1 + ((TIMESTAMP)(discharge_lockout_time));
			}
			//Default else - IDLE has no such restrictions
		}

		//Dispatch change, but same mode
		if (new_lf_dispatch_power != lf_dispatch_power)
		{
			//See if it is appreciable - just in case - I'm artibrarily declaring milliWatts the threshold
			if (fabs(new_lf_dispatch_power - lf_dispatch_power)>=0.001)
			{
				//Put the new power value in
				lf_dispatch_power = new_lf_dispatch_power;

				//Force a reiteration
				t2 = t1;

				//Update lockout allowed -- This probably needs to be refined so if in deadband, incremental changes can still occur - TO DO
				if (new_lf_status == CHARGE)
				{
					//Hard lockout for now
					lf_dispatch_change_allowed = false;

					//Apply update time
					next_update_time = t1 + ((TIMESTAMP)(charge_lockout_time));
				}
				else if (new_lf_status == DISCHARGE)
				{
					//Hard lockout for now
					lf_dispatch_change_allowed = false;

					//Apply update time
					next_update_time = t1 + ((TIMESTAMP)(discharge_lockout_time));
				}
				//Default else - IDLE has no such restrictions
			}
			//Defaulted else - change is less than 0.001 W, so who cares
		}
		//Default else, do nothing
	}//End LOAD_FOLLOWING redispatch

	if (inverter_type_v != FOUR_QUADRANT)	//Remove contributions for XML properness
	{
		if ((phases & 0x10) == 0x10)	//Triplex
		{
			*pLine12 -= last_current[3];	//Remove from current12
		}
		else	//Some variant of three-phase
		{
			//Remove our parent contributions (so XMLs look proper)
			pLine_I[0] -= last_current[0];
			pLine_I[1] -= last_current[1];
			pLine_I[2] -= last_current[2];
		}
	}
	else	//Must be four quadrant (load_following or otherwise)
	{
		if ((phases & 0x10) == 0x10)	//Triplex
		{
			*pPower -= last_power[3];	//Theoretically pPower is mapped to power_12, which already has the [2] offset applied
		}
		else	//Variation of three-phase
		{
			pPower[0] -= last_power[0];
			pPower[1] -= last_power[1];
			pPower[2] -= last_power[2];
		}
	}
	
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

bool *inverter::get_bool(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_bool)
		return NULL;
	return (bool*)GETADDR(obj,p);
}
int *inverter::get_enum(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_enumeration)
		return NULL;
	return (int*)GETADDR(obj,p);
}
complex * inverter::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_inverter(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(inverter::oclass);
		if (*obj!=NULL)
		{
			inverter *my = OBJECTDATA(*obj,inverter);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(inverter);
}

EXPORT int init_inverter(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,inverter)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(inverter);
}

EXPORT TIMESTAMP sync_inverter(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	inverter *my = OBJECTDATA(obj,inverter);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;		
	}
	SYNC_CATCHALL(inverter);
	return t2;
}

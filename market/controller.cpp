/** $Id: controller.cpp 1182 2009-09-09 22:08:36Z mhauer $
	Copyright (C) 2009 Battelle Memorial Institute
	@file auction.cpp
	@defgroup controller Transactive controller, OlyPen experiment style
	@ingroup market

 **/

#include "controller.h"

EXPORT int notify_controller_proxy_bid_ready(OBJECT *obj, char *value);
	
CLASS* controller::oclass = NULL;

controller::controller(MODULE *module){
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"controller",sizeof(controller),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==NULL)
			throw "unable to register class controller";
		else
			oclass->trl = TRL_QUALIFIED;

		if (gl_publish_variable(oclass,
			PT_enumeration, "simple_mode", PADDR(simplemode),
				PT_KEYWORD, "NONE", SM_NONE,
				PT_KEYWORD, "HOUSE_HEAT", SM_HOUSE_HEAT,
				PT_KEYWORD, "HOUSE_COOL", SM_HOUSE_COOL,
				PT_KEYWORD, "HOUSE_PREHEAT", SM_HOUSE_PREHEAT,
				PT_KEYWORD, "HOUSE_PRECOOL", SM_HOUSE_PRECOOL,
				PT_KEYWORD, "WATERHEATER", SM_WATERHEATER,
				PT_KEYWORD, "DOUBLE_RAMP", SM_DOUBLE_RAMP,
			PT_enumeration, "bid_mode", PADDR(bidmode),
				PT_KEYWORD, "ON", BM_ON,
				PT_KEYWORD, "OFF", BM_OFF,
				PT_KEYWORD, "PROXY", BM_PROXY,
			PT_enumeration, "use_override", PADDR(use_override),
				PT_KEYWORD, "OFF", OU_OFF,
				PT_KEYWORD, "ON", OU_ON,
			PT_enumeration, "proxy_state", PADDR(proxystate),
				PT_KEYWORD, "NOTREADY", PS_NOTREADY,
				PT_KEYWORD, "INIT", PS_INIT,
				PT_KEYWORD, "READY", PS_READY,
				PT_KEYWORD, "ERROR", PS_ERROR,
			PT_enumeration, "proxy_bid_result", PADDR(bid_result),
				PT_KEYWORD, "NONE", PR_NONE,
				PT_KEYWORD, "SENT", PR_SENT,
				PT_KEYWORD, "SUCCESS", PR_SUCCESS,
				PT_KEYWORD, "FAIL_LATE", PR_FAIL_LATE,
				PT_KEYWORD, "FAIL_BOGUS", PR_FAIL_BOGUS,
				PT_KEYWORD, "FAIL_WARMUP", PR_FAIL_WARMUP,
				PT_KEYWORD, "FAIL_EARLY", PR_FAIL_EARLY,
				PT_KEYWORD, "FAIL_ZEROQ", PR_FAIL_ZEROQ,
				PT_KEYWORD, "FAIL_BADBIDNUM", PR_FAIL_BADBIDNUM,
			PT_double, "ramp_low[degF]", PADDR(ramp_low), PT_DESCRIPTION, "the comfort response below the setpoint",
			PT_double, "ramp_high[degF]", PADDR(ramp_high), PT_DESCRIPTION, "the comfort response above the setpoint",
			PT_double, "range_low", PADDR(range_low), PT_DESCRIPTION, "the setpoint limit on the low side",
			PT_double, "range_high", PADDR(range_high), PT_DESCRIPTION, "the setpoint limit on the high side",
			PT_char32, "target", PADDR(target), PT_DESCRIPTION, "the observed property (e.g., air temperature)",
			PT_char32, "setpoint", PADDR(setpoint), PT_DESCRIPTION, "the controlled property (e.g., heating setpoint)",
			PT_char32, "demand", PADDR(demand), PT_DESCRIPTION, "the controlled load when on",
			PT_char32, "load", PADDR(load), PT_DESCRIPTION, "the current controlled load",
			PT_char32, "total", PADDR(total), PT_DESCRIPTION, "the uncontrolled load (if any)",
			PT_object, "market", PADDR(pMarket), PT_DESCRIPTION, "the market to bid into",
			PT_char32, "state", PADDR(state), PT_DESCRIPTION, "the state property of the controlled load",
			PT_char32, "avg_target", PADDR(avg_target),
			PT_char32, "std_target", PADDR(std_target),
			PT_double, "bid_price", PADDR(last_p), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid price",
			PT_double, "bid_quantity", PADDR(last_q), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid quantity",
//			PT_enumeration, "bid_state", PADDR(last_state), PT_ACCESS, PA_REFERENCE,
//				PT_KEYWORD, "UNKNOWN", BS_UNKNOWN,
//				PT_KEYWORD, "ON", BS_ON,
//				PT_KEYWORD, "OFF", BS_OFF,
			PT_double, "set_temp[degF]", PADDR(set_temp), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the reset value",
			PT_double, "base_setpoint[degF]", PADDR(setpoint0),
			// new stuff
			PT_double, "period[s]", PADDR(dPeriod), PT_DESCRIPTION, "interval of time between market clearings",
			PT_enumeration, "control_mode", PADDR(control_mode),
				PT_KEYWORD, "RAMP", CN_RAMP,
				PT_KEYWORD, "DOUBLE_RAMP", CN_DOUBLE_RAMP,
			PT_enumeration, "resolve_mode", PADDR(resolve_mode),
				PT_KEYWORD, "DEADBAND", RM_DEADBAND,
				PT_KEYWORD, "SLIDING", RM_SLIDING,
				PT_KEYWORD, "DOMINANT", RM_DOMINANT,
			PT_double, "slider_setting",PADDR(slider_setting),
			PT_double, "slider_setting_heat", PADDR(slider_setting_heat),
			PT_double, "slider_setting_cool", PADDR(slider_setting_cool),
			PT_char32, "override", PADDR(re_override),
			// double ramp
			PT_double, "heating_range_high[degF]", PADDR(heat_range_high),
			PT_double, "heating_range_low[degF]", PADDR(heat_range_low),
			PT_double, "heating_ramp_high", PADDR(heat_ramp_high),
			PT_double, "heating_ramp_low", PADDR(heat_ramp_low),
			PT_double, "cooling_range_high[degF]", PADDR(cool_range_high),
			PT_double, "cooling_range_low[degF]", PADDR(cool_range_low),
			PT_double, "cooling_ramp_high", PADDR(cool_ramp_high),
			PT_double, "cooling_ramp_low", PADDR(cool_ramp_low),
			PT_double, "heating_base_setpoint[degF]", PADDR(heating_setpoint0),
			PT_double, "cooling_base_setpoint[degF]", PADDR(cooling_setpoint0),
			PT_char32, "deadband", PADDR(deadband),
			PT_char32, "heating_setpoint", PADDR(heating_setpoint),
			PT_char32, "heating_demand", PADDR(heating_demand),
			PT_char32, "cooling_setpoint", PADDR(cooling_setpoint),
			PT_char32, "cooling_demand", PADDR(cooling_demand),
			PT_double, "sliding_time_delay[s]", PADDR(sliding_time_delay), PT_DESCRIPTION, "time interval desired for the sliding resolve mode to change from cooling or heating to off",
			PT_bool, "bid_deadband_ends", PADDR(bid_deadband_ends),
			// redefinitions
			PT_char32, "average_target", PADDR(avg_target),
			PT_char32, "standard_deviation_target", PADDR(std_target),
			//#ifdef _DEBUG
			PT_enumeration, "current_mode", PADDR(thermostat_mode),
				PT_KEYWORD, "INVALID", TM_INVALID,
				PT_KEYWORD, "OFF", TM_OFF,
				PT_KEYWORD, "HEAT", TM_HEAT,
				PT_KEYWORD, "COOL", TM_COOL,
			PT_enumeration, "dominant_mode", PADDR(last_mode),
				PT_KEYWORD, "INVALID", TM_INVALID,
				PT_KEYWORD, "OFF", TM_OFF,
				PT_KEYWORD, "HEAT", TM_HEAT,
				PT_KEYWORD, "COOL", TM_COOL,
			PT_enumeration, "previous_mode", PADDR(previous_mode),
				PT_KEYWORD, "INVALID", TM_INVALID,
				PT_KEYWORD, "OFF", TM_OFF,
				PT_KEYWORD, "HEAT", TM_HEAT,
				PT_KEYWORD, "COOL", TM_COOL,
			PT_double, "heat_max", PADDR(heat_max),
			PT_double, "cool_min", PADDR(cool_min),
			PT_double, "heat_min", PADDR(heat_min),
			PT_double, "cool_max", PADDR(cool_max),
			PT_double, "min", PADDR(min),
			PT_double, "max", PADDR(max),
			
		//#endif
			PT_int32, "bid_delay", PADDR(bid_delay),
			PT_enumeration, "bid_state", PADDR(last_pState), // see residential_enduse
				PT_KEYWORD, "ON", 1,
				PT_KEYWORD, "OFF", 0,
				PT_KEYWORD, "UNKNOWN", -1,
			PT_int64, "bid_id", PADDR(lastbid_id),
			// for proxy mode
			PT_object, "proxy_object", PADDR(proxy_object),
			PT_bool, "proxy_bid_ready", PADDR(proxy_bid_ready), PT_HAS_NOTIFY,
			// invariant proxy values
			PT_double, "proxy_init_price", PADDR(proxy_init_price),
			PT_int32, "proxy_market_period", PADDR(proxy_market_period),
			PT_char32, "proxy_market_unit", PADDR(proxy_market_unit),
			// variant proxy values
			PT_int64, "proxy_market_id", PADDR(proxy_market_id),
			PT_int64, "proxy_bid_id", PADDR(proxy_bid_id), PT_HAS_NOTIFY,
			PT_int32, "proxy_bid_retval", PADDR(proxy_bid_retval),
			PT_timestamp, "proxy_clear_time", PADDR(proxy_clear_time), PT_HAS_NOTIFY,
			PT_double, "proxy_clear_price", PADDR(proxy_clear_price),
			PT_double, "proxy_marginal_frac", PADDR(proxy_marginal_frac),
			PT_double, "proxy_price_cap", PADDR(proxy_price_cap),
			PT_double, "proxy_avg", PADDR(proxy_avg),
			PT_double, "proxy_stdev", PADDR(proxy_stdev),
			PT_int32, "proxy_delay", PADDR(proxy_delay),
			PT_timestamp, "proxy_last_tx", PADDR(proxy_last_tx),
			PT_int32, "no_bid_reason", PADDR(no_bid_reason),
			PT_double, "bid_return_check[s]", PADDR(bid_return_check),
			PT_int32, "bid_response", PADDR(bid_resp),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(controller));
	}
}

int controller::create(){
	memset(this, 0, sizeof(controller));
	sprintf(avg_target, "avg24");
	sprintf(std_target, "std24");
	slider_setting_heat = -0.001;
	slider_setting_cool = -0.001;
	slider_setting = -0.001;
	sliding_time_delay = -1;
	lastbid_id = 0;
	heat_range_low = -5;
	heat_range_high = 3;
	cool_range_low = -3;
	cool_range_high = 5;
	use_override = OU_OFF;
	period = 0;
	proxystate = PS_NOTREADY;
	bid_setpoint_change = false;
	proxy_market_id = -1;
	bidmode = BM_ON;
	proxy_last_tx = 0;
	proxy_delay = 0;
	bid_count = 0;
	last_bid_count = 0;
	bid_return = 0;
	return 1;
}

/** provides some easy default inputs for the transactive controller,
	 and some examples of what various configurations would look like.
 **/
void controller::cheat(){
	switch(simplemode){
		case SM_NONE:
			break;
		case SM_HOUSE_HEAT:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "heating_setpoint");
			sprintf(demand, "heating_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			ramp_low = -2;
			ramp_high = -2;
			range_low = -5;
			range_high = 0;
			dir = -1;
			break;
		case SM_HOUSE_COOL:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "cooling_setpoint");
			sprintf(demand, "cooling_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			ramp_low = 2;
			ramp_high = 2;
			range_low = 0;
			range_high = 5;
			dir = 1;
			break;
		case SM_HOUSE_PREHEAT:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "heating_setpoint");
			sprintf(demand, "heating_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			ramp_low = -2;
			ramp_high = -2;
			range_low = -5;
			range_high = 3;
			dir = -1;
			break;
		case SM_HOUSE_PRECOOL:
			sprintf(target, "air_temperature");
			sprintf(setpoint, "cooling_setpoint");
			sprintf(demand, "cooling_demand");
			sprintf(total, "total_load");
			sprintf(load, "hvac_load");
			sprintf(state, "power_state");
			ramp_low = 2;
			ramp_high = 2;
			range_low = -3;
			range_high = 5;
			dir = 1;
			break;
		case SM_WATERHEATER:
			sprintf(target, "temperature");
			sprintf(setpoint, "tank_setpoint");
			sprintf(demand, "heating_element_capacity");
			sprintf(total, "actual_load");
			sprintf(load, "actual_load");
			sprintf(state, "power_state");
			ramp_low = -2;
			ramp_high = -2;
			range_low = 0;
			range_high = 10;
			break;
		case SM_DOUBLE_RAMP:
			control_mode = CN_DOUBLE_RAMP;
			sprintf(target, "air_temperature");
			sprintf(heating_setpoint, "heating_setpoint");
			sprintf(heating_demand, "heating_demand");
			sprintf(heating_total, "total_load");		// using total instead of heating_total
			sprintf(heating_load, "hvac_load");			// using load instead of heating_load
			sprintf(cooling_setpoint, "cooling_setpoint");
			sprintf(cooling_demand, "cooling_demand");
			sprintf(cooling_total, "total_load");		// using total instead of cooling_total
			sprintf(cooling_load, "hvac_load");			// using load instead of cooling_load
			sprintf(deadband, "thermostat_deadband");
			sprintf(load, "hvac_load");
			sprintf(total, "total_load");
			sprintf(state, "power_state");
			heat_ramp_low = -2;
			heat_ramp_high = -2;
			heat_range_low = -5;
			heat_range_high = 5;
			cool_ramp_low = 2;
			cool_ramp_high = 2;
			cool_range_low = 5;
			cool_range_high = 5;
			break;
		default:
			break;
	}
}


/** convenience shorthand
 **/
void controller::fetch(double **prop, char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_double_by_name(parent, name);
	if(*prop == NULL){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "controller:%i", hdr->id);
		if(*name == NULL)
			sprintf(msg, "%s: controller unable to find property: name is NULL", namestr);
		else
			sprintf(msg, "%s: controller unable to find %s", namestr, name);
		throw(msg);
	}
}

int controller::init_direct(OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	char tname[32];
	char *namestr = (hdr->name ? hdr->name : tname);
//	double high, low;

	sprintf(tname, "controller:%i", hdr->id);

	if(pMarket == NULL){
		gl_error("%s: controller has no market, therefore no price signals", namestr);
		return 0;
	} else if(gl_object_isa(pMarket, "auction")){
		gl_set_dependent(hdr, pMarket);
		market = OBJECTDATA(pMarket, auction);
	} else {
		gl_error("direct controllers only work when attached to an 'auction' object");
		return 0;
	}

	if(dPeriod == 0.0){
		period = market->period;
	} else {
		period = (TIMESTAMP)floor(dPeriod + 0.5);
	}

	if(bid_delay > period){
		gl_warning("Bid delay is greater than the controller period. Resetting bid delay to 0.");
		bid_delay = 0;
	}

	
	fetch(&pAvg, avg_target, pMarket);
	fetch(&pStd, std_target, pMarket);

	last_p = market->init_price;

	// fill in proxy values
	proxy_object = pMarket;
	proxy_init_price = market->init_price;
	proxy_market_period = period;
	memcpy(proxy_market_unit, market->unit, 32);
	proxy_market_id = market->market_id;
	proxy_clear_time = TS_NEVER;
	proxy_clear_price = last_p;
	proxy_marginal_frac = 0.0;
	proxy_price_cap = market->pricecap;
	proxy_avg = *pAvg;
	proxy_stdev = *pStd;

	proxystate = PS_READY;

	if(0 > proxy_delay){ // negative check
		proxy_delay = 0;
	}
	return 1;
}

int controller::init_proxy(OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	char tname[32];
	char *namestr = (hdr->name ? hdr->name : tname);
	sprintf(tname, "controller:%i", hdr->id);

	if(pMarket != NULL){
		gl_warning("init_proxy(): %s has defined a market object while using proxy mode", namestr);
	}

	if(dPeriod == 0.0){
		gl_error("init_proxy(): %s has not defined a market period");
		return 0;
	} else {
		period = (TIMESTAMP)floor(dPeriod + 0.5);
	}
	if(bid_delay > period){
		gl_warning("Bid delay is greater than the controller period. Resetting bid delay to 0.");
		bid_delay = 0;
	}

	proxystate = PS_INIT;
	proxy_bid_id = -1;
	proxy_market_id = -1;

	return 1;
}

/** initialization process
 **/
int controller::init(OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	char tname[32];
	char *namestr = (hdr->name ? hdr->name : tname);
//	double high, low;

	sprintf(tname, "controller:%i", hdr->id);

	cheat();

	if(parent == NULL){
		gl_error("%s: controller has no parent, therefore nothing to control", namestr);
		return 0;
	}

	if(target[0] == 0){
		GL_THROW("controller: %i, target property not specified", hdr->id);
	}
	if(setpoint[0] == 0 && control_mode == CN_RAMP){
		GL_THROW("controller: %i, setpoint property not specified", hdr->id);;
	}
	if(demand[0] == 0 && control_mode == CN_RAMP){
		GL_THROW("controller: %i, demand property not specified", hdr->id);
	}
	if(total[0] == 0){
		GL_THROW("controller: %i, total property not specified", hdr->id);
	}
	if(load[0] == 0){
		GL_THROW("controller: %i, load property not specified", hdr->id);
	}

	if(heating_setpoint[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, heating_setpoint property not specified", hdr->id);;
	}
	if(heating_demand[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, heating_demand property not specified", hdr->id);
	}

	if(cooling_setpoint[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, cooling_setpoint property not specified", hdr->id);;
	}
	if(cooling_demand[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, cooling_demand property not specified", hdr->id);
	}

	if(deadband[0] == 0 && control_mode == CN_DOUBLE_RAMP){
		GL_THROW("controller: %i, deadband property not specified", hdr->id);
	}

	fetch(&pMonitor, target, parent);
	if(control_mode == CN_RAMP){
		fetch(&pSetpoint, setpoint, parent);
		fetch(&pDemand, demand, parent);
		fetch(&pTotal, total, parent);
		fetch(&pLoad, load, parent);
		if (bid_deadband_ends == true)
			fetch(&pDeadband, deadband, parent);
	} else if(control_mode == CN_DOUBLE_RAMP){
		sprintf(aux_state, "is_AUX_on");
		sprintf(heat_state, "is_HEAT_on");
		sprintf(cool_state, "is_COOL_on");
		fetch(&pHeatingSetpoint, heating_setpoint, parent);
		fetch(&pHeatingDemand, heating_demand, parent);
		fetch(&pHeatingTotal, total, parent);
		fetch(&pHeatingLoad, total, parent);
		fetch(&pCoolingSetpoint, cooling_setpoint, parent);
		fetch(&pCoolingDemand, cooling_demand, parent);
		fetch(&pCoolingTotal, total, parent);
		fetch(&pCoolingLoad, load, parent);
		fetch(&pDeadband, deadband, parent);
		fetch(&pAuxState, aux_state, parent);
		fetch(&pHeatState, heat_state, parent);
		fetch(&pCoolState, cool_state, parent);
	}

	if(parent == NULL){
		gl_error("%s: controller has no parent, therefore nothing to control", namestr);
		return 0;
	}

	if(dir == 0){
		double high = ramp_high * range_high;
		double low = ramp_low * range_low;
		if(high > low){
			dir = 1;
		} else if(high < low){
			dir = -1;
		} else if((high == low) && (fabs(ramp_high) > 0.001 || fabs(ramp_low) > 0.001)){
			dir = 0;
			if (ramp_high > 0) //cooling
				dir2 = 1;
			else
				dir2 = -1;
			gl_warning("%s: controller has no price ramp", namestr);
			/* occurs given no price variation, or no control width (use a normal thermostat?) */
		}
		if(ramp_low * ramp_high < 0){
			gl_warning("%s: controller price curve is not injective and may behave strangely");
			/* TROUBLESHOOTING
				The price curve 'changes directions' at the setpoint, which may create odd
				conditions in a number of circumstances.
			 */
		}
	}
	if(setpoint0==0)
		setpoint0 = -1; // key to check first thing

	if(heating_setpoint0==0)
		heating_setpoint0 = -1;

	if(cooling_setpoint0==0)
		cooling_setpoint0 = -1;

//	double period = market->period;
//	next_run = gl_globalclock + (TIMESTAMP)(period - fmod(gl_globalclock+period,period));
	next_run = gl_globalclock;// + (market->period - gl_globalclock%market->period);
	time_off = TS_NEVER;
	if(sliding_time_delay < 0 ) {
		dtime_delay = 21600; // default sliding_time_delay of 6 hours
	} else {
		dtime_delay = (int64)sliding_time_delay;
	}

	if(state[0] != 0){
		// grab state pointer
		pState = gl_get_enum_by_name(parent, state);
		last_pState = 0;
		if(pState == 0){
			gl_error("state property name \'%s\' is not published by parent class", state);
			return 0;
		}
	}

	if(heating_state[0] != 0){
		// grab state pointer
		pHeatingState = gl_get_enum_by_name(parent, heating_state);
		if(pHeatingState == 0){
			gl_error("heating_state property name \'%s\' is not published by parent class", heating_state.get_string());
			return 0;
		}
	}

	if(cooling_state[0] != 0){
		// grab state pointer
		pCoolingState = gl_get_enum_by_name(parent, cooling_state);
		if(pCoolingState == 0){
			gl_error("cooling_state property name \'%s\' is not published by parent class", cooling_state.get_string());
			return 0;
		}
	}
	// get override, if set
	if(re_override[0] != 0){
		pOverride = gl_get_enum_by_name(parent, re_override);
	}
	if((pOverride == 0) && (use_override == OU_ON)){
		gl_error("use_override is ON but no valid override property name is given");
		return 0;
	}

	if(bid_delay < 0){
		bid_delay = -bid_delay;
	}
	if(control_mode == CN_RAMP){
		if(slider_setting < -0.001){
		gl_warning("slider_setting is negative, reseting to 0.0");
		slider_setting = 0.0;
	}
	if(slider_setting > 1.0){
		gl_warning("slider_setting is greater than 1.0, reseting to 1.0");
		slider_setting = 1.0;
	}
	}
	if(control_mode == CN_DOUBLE_RAMP){
		if(slider_setting_heat < -0.001){
			gl_warning("slider_setting_heat is negative, reseting to 0.0");
			slider_setting_heat = 0.0;
		}
		if(slider_setting_cool < -0.001){
			gl_warning("slider_setting_cool is negative, reseting to 0.0");
			slider_setting_cool = 0.0;
		}
		if(slider_setting_heat > 1.0){
			gl_warning("slider_setting_heat is greater than 1.0, reseting to 1.0");
			slider_setting_heat = 1.0;
		}
		if(slider_setting_cool > 1.0){
			gl_warning("slider_setting_cool is greater than 1.0, reseting to 1.0");
			slider_setting_cool = 1.0;
		}
		// get override, if set
	}

	// proxy or non-proxy specific init()
	if(bidmode == BM_PROXY){
		return init_proxy(parent);
	}

	if(bidmode == BM_ON || bidmode == BM_OFF){
		return init_direct(parent);
	}
	return 1;
}


int controller::isa(char *classname)
{
	return strcmp(classname,"controller")==0;
}


TIMESTAMP controller::presync(TIMESTAMP t0, TIMESTAMP t1){
	if(slider_setting < -0.001)
		slider_setting = 0.0;
	if(slider_setting_heat < -0.001)
		slider_setting_heat = 0.0;
	if(slider_setting_cool < -0.001)
		slider_setting_cool = 0.0;
	if(slider_setting > 1.0)
		slider_setting = 1.0;
	if(slider_setting_heat > 1.0)
		slider_setting_heat = 1.0;
	if(slider_setting_cool > 1.0)
		slider_setting_cool = 1.0;

	if(control_mode == CN_RAMP && setpoint0 == -1)
		setpoint0 = *pSetpoint;
	if(control_mode == CN_DOUBLE_RAMP && heating_setpoint0 == -1)
		heating_setpoint0 = *pHeatingSetpoint;
	if(control_mode == CN_DOUBLE_RAMP && cooling_setpoint0 == -1)
		cooling_setpoint0 = *pCoolingSetpoint;

	//if(t1 == next_run || t0 == 0){
		if(control_mode == CN_RAMP){
			if (slider_setting == -0.001) { // don't use the slider
				min = setpoint0 + range_low;
				max = setpoint0 + range_high;
			}
			else if(slider_setting > 0) { // non-zero slider
				min = setpoint0 + range_low * slider_setting;
				max = setpoint0 + range_high * slider_setting;
				if(range_low != 0)
					ramp_low = -2 - (1 - slider_setting);
				else
					ramp_low = 0;
				if(range_high != 0)
					ramp_high = 2 + (1 - slider_setting);
				else
					ramp_high = 0;
			} else {
				min = setpoint0;
				max = setpoint0;
			}
		} else if(control_mode == CN_DOUBLE_RAMP){
			if (slider_setting_cool == -0.001) { // don't use the slider
				cool_min = cooling_setpoint0 + cool_range_low;
				cool_max = cooling_setpoint0 + cool_range_high;
			}
			else if (slider_setting_cool > 0.0) { // use the slider
				cool_min = cooling_setpoint0 + cool_range_low * slider_setting_cool;
				cool_max = cooling_setpoint0 + cool_range_high * slider_setting_cool;
				if (cool_range_low != 0.0)
					cool_ramp_low = -2 - (1 - slider_setting_cool);
				else
					cool_ramp_low = 0;
				if (cool_range_high != 0.0)
					cool_ramp_high = 2 + (1 - slider_setting_cool);
				else
					cool_ramp_high = 0;
			} else { // zero slider
				cool_min = cooling_setpoint0;
				cool_max = cooling_setpoint0;
			}
			if (slider_setting_heat == -0.001) { // don't use the slider
				heat_min = heating_setpoint0 + heat_range_low;
				heat_max = heating_setpoint0 + heat_range_high;
			}
			else if (slider_setting_heat > 0.0) { // use the slider
				heat_min = heating_setpoint0 + heat_range_low * slider_setting_heat;
				heat_max = heating_setpoint0 + heat_range_high * slider_setting_heat;
				if (heat_range_low != 0.0)
					heat_ramp_low = 2 + (1 - slider_setting_heat);
				else
					heat_ramp_low = 0;
				if (heat_range_high != 0)
					heat_ramp_high = -2 - (1 - slider_setting_heat);
				else
					heat_ramp_high = 0;
			} else { // slider is set to zero
				heat_min = heating_setpoint0;
				heat_max = heating_setpoint0;
			}
		}
		if((thermostat_mode != TM_INVALID && thermostat_mode != TM_OFF) || t1 >= time_off)
			if(resolve_mode != RM_DOMINANT)
				last_mode = thermostat_mode;
		else if(thermostat_mode == TM_INVALID)
			last_mode = TM_OFF;// this initializes last mode to off

		if(thermostat_mode != TM_INVALID)
			previous_mode = thermostat_mode;
		else
			previous_mode = TM_OFF;
	//}
	return TS_NEVER;
}

TIMESTAMP controller::ramp_control(TIMESTAMP t0, TIMESTAMP t1){
	double bid = -1.0;
	int64 no_bid = 0; // flag gets set when the current temperature drops in between the the heating setpoint and cooling setpoint curves
	double demand = 0.0;
	double rampify = 0.0;
	extern double bid_offset;
	OBJECT *hdr = OBJECTHDR(this);
	char32 namestr;
	bool market_has_changed = false, bid_has_changed = false, demand_has_changed = false;
	
	// set up some variables for the deadband offset
	double db_offset = 0;
	
	if (bid_deadband_ends)
	{
		db_offset = *pDeadband / 2;
	}
	
	// if market has updated, continue onwards
	if(proxy_market_id != lastmkt_id){// && (*pAvg == 0.0 || *pStd == 0.0 || setpoint0 == 0.0)){
		double clear_price;
		gl_name(hdr, namestr, 31);
//		gl_output("%s is running ramp_control(%lu, %lu)", namestr, t0, t1);
//		lastmkt_id = market->market_id;
		lastmkt_id = proxy_market_id;
		if(bidmode == BM_PROXY){
		  lastbid_id = proxy_bid_id = 0;
		  proxy_has_id = false;
		  last_bid_count = bid_count;	
		} else {
		  lastbid_id = 0; // clear last bid id, refers to an old market
		}
		  
		// update using last price
		// T_set,a = T_set + (P_clear - P_avg) * | T_lim - T_set | / (k_T * stdev24)

		//clear_price = market->current_frame.clearing_price;
		clear_price = proxy_clear_price;
		double offset_sign = 0;
		if (bid_deadband_ends)
		{
			if ( (dir > 0 && clear_price < last_p) || (dir < 0 && clear_price > last_p) )
				offset_sign = -1;
			else if ( (dir > 0 && clear_price >= last_p) || (dir < 0 && clear_price <= last_p))
				offset_sign = 1;
			else
				offset_sign = 0;
		}
		if(fabs(proxy_stdev) < bid_offset){ // "close enough to zero"
			set_temp = setpoint0;
		//} else if(clear_price < *pAvg && range_low != 0){
		} else if(clear_price < proxy_avg && range_low != 0){
//			set_temp = setpoint0 + (clear_price - *pAvg) * fabs(range_low) / (ramp_low * *pStd);
			set_temp = setpoint0 + (clear_price - proxy_avg) * fabs(range_low) / (ramp_low * proxy_stdev) + db_offset*offset_sign;
//		} else if(clear_price > *pAvg && range_high != 0){
		} else if(clear_price > proxy_avg && range_high != 0){
//			set_temp = setpoint0 + (clear_price - *pAvg) * fabs(range_high) / (ramp_high * *pStd);
			set_temp = setpoint0 + (clear_price - proxy_avg) * fabs(range_high) / (ramp_high * proxy_stdev) + db_offset*offset_sign;
		} else {
			set_temp = setpoint0 + db_offset*offset_sign;
		}

		if((use_override == OU_ON) && (pOverride != 0)){
			if(clear_price <= last_p){
				// if we're willing to pay as much as, or for more than the offered price, then run.
				*pOverride = 1;
			} else {
				*pOverride = -1;
			}
		}

		// clip
		if(set_temp > max){
			set_temp = max;
		} else if(set_temp < min){
			set_temp = min;
		}

		*pSetpoint = set_temp;
		//gl_verbose("controller::postsync(): temp %f given p %f vs avg %f",set_temp, market->next.price, market->avg24);
	} else {//we are trying to rebid
		if(bidmode == BM_PROXY){
			if(bid_count > last_bid_count){//we have made a bid into the market for the current market.
				if(proxy_has_id == false){
					proxy_bid_ready = false;
					gl_warning("The market did not assign the controller a bid_id yet. Waiting for market.");
					no_bid_reason = 4;//The controller didn't bid because it's still waiting to recieve confirmation of it's previous bid.
					if(bid_return_check > 0){
						bid_return = ((TIMESTAMP)(bid_return_check)) + t1;
						return bid_return;
					} else {
						return TS_NEVER;
					}
				}
			}
		}	
	}

	if(dir > 0){
		if (bid_deadband_ends)
		{
			if(*pState == 0 && *pMonitor > (max - db_offset)){
			
				bid = proxy_price_cap;
			}
			//else if (*pState != 0 && *pMonitor > (max + db_offset)) {
			//	
			//	bid = proxy_price_cap;
			//}
			//else if (*pState == 0 && *pMonitor < (min - db_offset)){
			//	//(*pState == 0 && *pMonitor < (min + db_offset)){
			//	bid = 0.0;
			//	no_bid = 1;
			//}
			else if (*pState != 0 && *pMonitor < (min + db_offset)){
				//(*pState != 0 && *pMonitor < (min - db_offset)){
				bid = 0.0;
				no_bid = 1;
			}
			else if(*pState != 0 && *pMonitor > (max)){
				bid = proxy_price_cap;
			} else if (*pState == 0 && *pMonitor < (min)){
				bid = 0.0;
				no_bid = 1;
			}

		}
		else
		{
			if(*pMonitor > (max)){
				bid = proxy_price_cap;
			} else if (*pMonitor < (min)){
				bid = 0.0;
				no_bid = 1;
			}
		}
	} else if(dir < 0){ //heating
		if (bid_deadband_ends)
		{
			if(*pState == 0 && *pMonitor < (min + db_offset)){
				bid = proxy_price_cap;
			}
			/*else if (*pState != 0 && *pMonitor < (min - db_offset)) {
				
				bid = proxy_price_cap;
			}
			else if (*pState == 0 && *pMonitor > (max + db_offset)){
				bid = 0.0;
				no_bid = 1;
			}*/
			else if (*pState != 0 && *pMonitor > (max - db_offset)){
				bid = 0.0;
				no_bid = 1;
			}
			else if(*pState != 0 && *pMonitor < (min)){
				bid = proxy_price_cap;
			} else if(*pState == 0 && *pMonitor > (max)){
				bid = 0.0;
				no_bid = 1;
			}
			
		}
		else
		{
			if(*pMonitor < (min)){
				bid = proxy_price_cap;
			} else if(*pMonitor > (max)){
				bid = 0.0;
				no_bid = 1;
			}
		}
	} else if(dir == 0){ // operation is "normal" but bidding, so always bid price_cap or nothing.
		if (bid_deadband_ends)
		{			
			// min and max should be equal here, so interchangeable
			//  dir2 used to determine whether in zero slope cooling or heating purely for hysteresis
			if (dir2 == 0.0) {
				gl_error("Variable dir2 did not get set correctly. Please notify d3x289 to correct.");
			}
			else if( (*pMonitor > max + db_offset || (*pState != 0 && *pMonitor > min - db_offset)) && dir2 > 0){ // want to cool at top of db or cooling until bottom
				bid = proxy_price_cap;
			} 
			else if( (*pMonitor < min - db_offset || (*pState != 0 && *pMonitor < max + db_offset)) && dir2 < 0){ // want to heat at bottom of db or heating until top
				bid = proxy_price_cap;
			} 
			else { // satisfied, so don't need to bid
				bid = 0.0;
				no_bid = 1;
			}
		}
		else
		{
			if(*pMonitor < min){
				bid = proxy_price_cap;
			} else if(*pMonitor > max){
				bid = 0.0;
				no_bid = 1;
			} else {
				bid = proxy_avg; // override due to lack of "real" curve
			}
		}
	}

	// calculate bid price
	if(*pMonitor > setpoint0){
		k_T = ramp_high;
		T_lim = range_high;
	} else if(*pMonitor < setpoint0) {
		k_T = ramp_low;
		T_lim = range_low;
	} else {
		k_T = 0.0;
		T_lim = 0.0;
	}

	if(bid < 0.0 && *pMonitor != setpoint0) {
		bid = proxy_avg + ( (fabs(proxy_stdev) < bid_offset) ? 0.0 : (*pMonitor - setpoint0) * (k_T * proxy_stdev) / fabs(T_lim) );
	} else if(*pMonitor == setpoint0) {
		bid = proxy_avg;
	}
	
	if(no_bid){
		last_p = 0;
		last_q = 0;
	}
	
	if(proxy_market_id != lastmkt_id){
		market_has_changed = true;
	}
	//if(fabs(bid-last_p) > 0.0075){
	//	bid_has_changed = true;
	//}
	//if(fabs(*pDemand - fabs(last_q)) > 0.1){
	//	demand_has_changed = true;
	//}
	// if our bid hasn't changed, don't re-send the same values.
//	if(!(market_has_changed || bid_has_changed || demand_has_changed)){
	if(proxy_market_id == lastmkt_id && bid == last_p && *pDemand == last_q){
		if(bidmode == BM_PROXY)
			proxy_bid_ready = false;
			no_bid_reason = 5;//The controller didn't bid becasue the bid was the same as the last bid for the current market.
		return TS_NEVER;
	} else {
		//gl_output("proxmkt %lli ?= lastmkit %lli, bid %g ?= last_p %g, pDmd %g ?= last_q %g", proxy_market_id, lastmkt_id, bid, last_p, *pDemand, last_q);
		if(!no_bid)
			gl_verbose("market %s change, dBid %g, dDemand (%g - %g)", (market_has_changed ? "DID" : "DID NOT"), fabs(bid-last_p), *pDemand,  fabs(last_q));
	}
	
	// bid the response part of the load
	double residual = *pTotal;
	/* WARNING ~ bid ID check will not work properly */
//	KEY bid_id = (KEY)(lastmkt_id == market->market_id ? lastbid_id : -1);
	KEY bid_id = 0;
	// 'if we know we have a bid ID, check if it's still valid, otherwise take note that we don't have one'
//	if(this->proxy_has_id){
//		bid_id = (KEY)(lastmkt_id == proxy_market_id ? lastbid_id : -1);
	bid_id = (KEY)(lastbid_id > 0 ? lastbid_id : -1);
//	} else {
//		bid_id = -1;
//	}
	// override
	//bid_id = -1;
	if(bidmode == BM_PROXY){
		if(proxy_delay > 0){
			if(t1 > proxy_last_tx + proxy_delay){
				// clear to re-send
				proxy_last_tx = t1;
			} else {
				// put off the bidding process
				// last_p/q are used for price-response...
				last_p = bid;
				last_q = -(*pDemand);
				gl_verbose("%s's is not bidding due to proxy_delay", hdr->name);
				proxy_bid_ready = false;
				no_bid_reason = 6;//The controller didn't bid because the proxy_delay hasn't be reached.
				return TS_NEVER;
			}
		}
	}
	if(*pDemand > 0 && no_bid != 1){
		last_p = bid;
		last_q = *pDemand;
//		if(0 != strcmp(market->unit, "")){
//			if(0 == gl_convert("kW", market->unit, &(last_q))){
//				gl_error("unable to convert bid units from 'kW' to '%s'", market->unit);
//				return TS_INVALID;
//			}
//		}
		if(0 != strcmp(proxy_market_unit, "")){
			if(0 == gl_convert("kW", proxy_market_unit, &(last_q))){
				gl_error("unable to convert bid units from 'kW' to '%s'", proxy_market_unit.get_string());
				return TS_INVALID;
			}
		}
		/*** SET PROXY BID VALUES HERE ***/
		//lastbid_id = market->submit(OBJECTHDR(this), -last_q, last_p, bid_id, (BIDDERSTATE)(pState != 0 ? *pState : 0));
		if(bidmode != BM_PROXY){
			if(pState != 0){
				last_pState = (*pState > 0 ? 1 : 0);
				lastbid_id = submit_bid_state(pMarket, hdr, -last_q, last_p, last_pState, bid_id);
			} else {
				last_pState = BS_UNKNOWN;
				lastbid_id = submit_bid(pMarket, hdr, -last_q, last_p, bid_id);
			}
			proxy_has_id = true;
		}  else {
			if(pState != 0){
				last_pState = (*pState > 0 ? 1 : 0);
			} else {
				last_pState = BS_UNKNOWN;
			}
			// copy the values to proxy values and wait for the proxy object to pick it up
			char nstr[64];
			last_q = -last_q; // make negative for bid reasons
			gl_verbose("%s proxy-bidding %f for %f kW (st %i, bid_id %lli)", gl_name(hdr, nstr, 63), last_p, last_q, last_pState, bid_id);
			proxy_bid_ready = true;
			bid_count += 1; 
			proxy_bid_id = bid_id;
			//notify_controller_proxy_bid_ready(hdr, "TRUE");
		}
		residual -= *pLoad;
	} else {
		last_p = 0;
		last_q = 0;
		no_bid_reason = 7;
		gl_verbose("%s's is not bidding", hdr->name);
	}
	if(residual < -0.001)
		gl_warning("controller:%d: residual unresponsive load is negative! (%.1f kW)", hdr->id, residual);

	return TS_NEVER;
}

TIMESTAMP controller::double_ramp_control(TIMESTAMP t0, TIMESTAMP t1){
	double bid = -1.0;
	int64 no_bid = 0; // flag gets set when the current temperature drops in between the the heating setpoint and cooling setpoint curves
	double demand = 0.0;
	double rampify = 0.0;
	extern double bid_offset;
	OBJECT *hdr = OBJECTHDR(this);
	char32 namestr;
	bool market_has_changed = false, bid_has_changed = false, demand_has_changed = false;
	double used_q, residual;
	KEY bid_id;

	/*
	double heat_range_high;
	double heat_range_low;
	double heat_ramp_high;
	double heat_ramp_low;
	double cool_range_high;
	double cool_range_low;
	double cool_ramp_high;
	double cool_ramp_low;
	*/

	// set up some variables for the deadband offset
	double db_offset = 0;
	
	if (bid_deadband_ends)
	{
		db_offset = *pDeadband / 2;
	}

	DATETIME t_next;
	gl_localtime(t1,&t_next);

	if(resolve_mode == RM_DOMINANT){

		if((lastHeatSetPoint != heating_setpoint0) || (lastCoolSetPoint != cooling_setpoint0))
		{
			if((heating_setpoint0  - *pDeadband/2) >= *pMonitor){
				cool_min = cooling_setpoint0;
				last_mode = TM_HEAT;
				
				if(heat_max > cooling_setpoint0 - *pDeadband) {
					gl_warning("Max heating setpoint is not less than a full deadband away from the cooling_base_setpoint. Resetting the max heating setpoint to be a full deadband less than the cooling_base_setpoint");
					heat_max = cool_min - *pDeadband;
				}

			}
			else if((cooling_setpoint0 + *pDeadband/2) <= *pMonitor){
				heat_max = heating_setpoint0;
				last_mode = TM_COOL;

				if(cool_min < heating_setpoint0 + *pDeadband) {
					gl_warning("Min cooling setpoint is not more than a full deadband away from the heating_base_setpoint. Resetting the min cooling setpoint to be a full deadband greater than the heating_base_setpoint");
					cool_min = heat_max + *pDeadband;							
				}

			}
			else{
				cool_min = cooling_setpoint0;
				heat_max = heating_setpoint0;
				last_mode = TM_OFF;
			}
			
		} else { 			
			if((heating_setpoint0  - *pDeadband/2) >= *pMonitor){

				cool_min = cooling_setpoint0;
				last_mode = TM_HEAT;	

				if(heat_max > cooling_setpoint0 - *pDeadband) {
					gl_warning("Max heating setpoint is not less than a full deadband away from the cooling_base_setpoint. Resetting the max heating setpoint to be a full deadband less than the cooling_base_setpoint");
					heat_max = cool_min - *pDeadband;
				}			
			}
			else if((cooling_setpoint0 + *pDeadband/2) <= *pMonitor){

				heat_max = heating_setpoint0;
				last_mode = TM_COOL;	

				if(cool_min < heating_setpoint0 + *pDeadband) {
					gl_warning("Min cooling setpoint is not more than a full deadband away from the heating_base_setpoint. Resetting the min cooling setpoint to be a full deadband greater than the heating_base_setpoint");
					cool_min = heat_max + *pDeadband;							
				}
			}

			if(TM_HEAT != last_mode && TM_COOL != last_mode){

				cool_min = cooling_setpoint0;
				heat_max = heating_setpoint0;
				last_mode = TM_OFF;
			}
			else if(last_mode == TM_HEAT){

				cool_min = cooling_setpoint0;
				if(heat_max > cooling_setpoint0 - *pDeadband) {
					heat_max = cool_min - *pDeadband;
				}
			}
			else if(last_mode == TM_COOL){

				heat_max = heating_setpoint0;
				if(cool_min < heating_setpoint0 + *pDeadband) {
					cool_min = heat_max + *pDeadband;							
				}
			}
		}					
			
	}
	if(bidmode != BM_PROXY){
		// fill proxy values with current values
		proxy_market_id = market->market_id;
		proxy_clear_price = market->current_frame.clearing_price;
		proxy_avg = *pAvg;
		proxy_stdev = *pStd;
	} else { // == BM_PROXY
		if(proxystate == PS_READY){
			// assume that the proxy values are good and have been updated 'recently'
			;
		} else {
			// assume that the proxy-mode controller is NOT ready to calculate bids,
			//	and short-circuit all this
			return TS_NEVER;
		}
	}

	// find crossover
	double midpoint = 0.0;
	if(cool_min - heat_max < *pDeadband){
		switch(resolve_mode){
			case RM_DEADBAND:
				midpoint = (heat_max + cool_min) / 2;
				if(midpoint - *pDeadband/2 < heating_setpoint0 || midpoint + *pDeadband/2 > cooling_setpoint0) {
					gl_error("The midpoint between the max heating setpoint and the min cooling setpoint must be half a deadband away from each base setpoint");
					return TS_INVALID;
				} else {
					heat_max = midpoint - *pDeadband/2;
					cool_min = midpoint + *pDeadband/2;
				}
				break;
			case RM_SLIDING:
				if(heat_max > cooling_setpoint0 - *pDeadband) {
					gl_error("the max heating setpoint must be a full deadband less than the cooling_base_setpoint");
					return TS_INVALID;
				}

				if(cool_min < heating_setpoint0 + *pDeadband) {
					gl_error("The min cooling setpoint must be a full deadband greater than the heating_base_setpoint");
					return TS_INVALID;
				}
				if(last_mode == TM_OFF || last_mode == TM_COOL){
					heat_max = cool_min - *pDeadband;
				} else if (last_mode == TM_HEAT){
					cool_min = heat_max + *pDeadband;
				}
				break;
			default:
				gl_error("unrecognized resolve_mode when double_ramp overlap resolution is needed");
				break;
		}
	}

	// if the market has updated,
//	if(lastmkt_id != market->market_id){
	if(proxy_market_id != lastmkt_id){
		double clear_price;

		gl_name(hdr, namestr, 31);

//		lastmkt_id = market->market_id;
		lastmkt_id = proxy_market_id;
		if(bidmode == BM_PROXY){
			lastbid_id = proxy_bid_id = 0;
			proxy_has_id = false;
		} else {
			lastbid_id = 0;
		}

		// retrieve cleared price
		
		//clear_price = market->current_frame.clearing_price;
		clear_price = proxy_clear_price;

		if(clear_price == last_p){
			// determine what to do at the marginal price
			switch(market->clearing_type){
				case CT_SELLER:	// may need to curtail
					break;
				case CT_PRICE:	// should not occur
				case CT_NULL:	// q zero or logic error ~ should not occur
					// occurs during the zero-eth market.
					//gl_warning("clearing price and bid price are equal with a market clearing type that involves inequal prices");
					break;
				default:
					break;
			}
		}
			double offset_sign = 0;
			//****FOR DOUBLE RAMP THIS IS HEATING AND COOLING**************
			if (bid_deadband_ends)
			{
				if ( (thermostat_mode == TM_COOL && clear_price < last_p) || (thermostat_mode == TM_HEAT && clear_price > last_p) )
					offset_sign = -1;
				else if ( (thermostat_mode == TM_COOL && clear_price >= last_p) || (thermostat_mode == TM_HEAT && clear_price <= last_p))
					offset_sign = 1;
				else
					offset_sign = 0;
			}
		may_run = 1;
		// calculate setpoints
		if(fabs(proxy_stdev) < bid_offset){ // "close enough to zero"
			set_temp_cool = cooling_setpoint0;
			set_temp_heat = heating_setpoint0;
		} else {
			//if(clear_price > *pAvg){
			if(clear_price > proxy_avg){
				//*pCoolingSetpoint = cooling_setpoint0 + (clear_price - *pAvg) * fabs(cool_range_high) / (cool_ramp_high * *pStd);
				set_temp_cool = cooling_setpoint0 + (clear_price - proxy_avg) * fabs(cool_range_high) / (cool_ramp_high * proxy_stdev) + db_offset*offset_sign;
				//*pHeatingSetpoint = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_range_low) / (heat_ramp_low * *pStd);
				set_temp_heat = heating_setpoint0 + (clear_price - proxy_avg) * fabs(heat_range_low) / (heat_ramp_low * proxy_stdev) + db_offset*offset_sign;
			//} else if(clear_price < *pAvg){
			} else if(clear_price < proxy_avg){
				//*pCoolingSetpoint = cooling_setpoint0 + (clear_price - *pAvg) * fabs(cool_range_low) / (cool_ramp_low * *pStd);
				set_temp_cool = cooling_setpoint0 + (clear_price - proxy_avg) * fabs(cool_range_low) / (cool_ramp_low * proxy_stdev) + db_offset*offset_sign;
				//*pHeatingSetpoint = heating_setpoint0 + (clear_price - *pAvg) * fabs(heat_range_high) / (heat_ramp_high * *pStd);
				set_temp_heat = heating_setpoint0 + (clear_price - proxy_avg) * fabs(heat_range_high) / (heat_ramp_high * proxy_stdev) + db_offset*offset_sign;
			} else {
					set_temp_cool = cooling_setpoint0 + db_offset*offset_sign;
				set_temp_heat = heating_setpoint0 + db_offset*offset_sign;
			}
		}

		// apply overrides
		if((use_override == OU_ON)){
			if(last_q != 0.0){
				if(clear_price == last_p && clear_price != proxy_price_cap){
					if(bidmode != BM_PROXY){
						if(market->margin_mode == AM_DENY){
							*pOverride = -1;
						} else if(market->margin_mode == AM_PROB){
							double r = gl_random_uniform(RNGSTATE,0, 1.0);
							if(r < market->current_frame.marginal_frac){
								*pOverride = 1;
							} else {
								*pOverride = -1;
							}
						}
					}
				} else if(clear_price <= last_p){				
					*pOverride = 1;
				} else {
					*pOverride = -1;
				}
			} else { // equality
				*pOverride = 0; // 'normal operation'
			}
		}

		//clip
		if(set_temp_cool > cool_max)
			set_temp_cool = cool_max;
		if(set_temp_cool < cool_min)
			set_temp_cool = cool_min;
		if(set_temp_heat > heat_max)
			set_temp_heat = heat_max;
		if(set_temp_heat < heat_min)
			set_temp_heat = heat_min;

		// this should really either lock of use a gl_set func.
		*pCoolingSetpoint = set_temp_cool;
		*pHeatingSetpoint = set_temp_heat;

		lastmkt_id = proxy_market_id;
	}
		
	// submit bids
	double previous_q = last_q; //store the last value, in case we need it
	bid = 0.0;
	used_q = 0.0;
	if (bid_deadband_ends){

		// We have to cool
		if(*pCoolState == 0 && *pMonitor > (cool_max - db_offset)){
				bid = proxy_price_cap;
				used_q = *pCoolingDemand;	
		}
		else if(*pCoolState != 0 && *pMonitor > (cool_max)) {
			//(*pCoolState != 0 && *pMonitor > (cool_max + db_offset)) 
				bid = proxy_price_cap;
				used_q = *pCoolingDemand;
		}
		// We have to heat
		else if(*pHeatState == 0 && *pMonitor < (heat_min + db_offset)){
				bid = proxy_price_cap;
				used_q = *pHeatingDemand;
		}
		else if(*pHeatState != 0 && *pMonitor < (heat_min)) {
			//(*pHeatState != 0 && *pMonitor < (heat_min - db_offset))
			bid = proxy_price_cap;
			used_q = *pHeatingDemand;
		}	
		// We're floating in between heating and cooling
		else if(*pCoolState == 0 && *pHeatState == 0 && (*pMonitor < (cool_min) && *pMonitor > (heat_max))){
			//(*pCoolState == 0 && *pHeatState == 0 && (*pMonitor < (cool_min - db_offset) && *pMonitor > (heat_max + db_offset)))
				bid = 0.0;
				used_q = 0.0;
		}
		else if((*pCoolState != 0 && (*pMonitor < (cool_min + db_offset)) || (*pHeatState != 0 && *pMonitor > (heat_max - db_offset)))){
				bid = 0.0;
				used_q = 0.0;
		}			
		// We might heat, if the price is right
		else if(((*pMonitor <= (heat_max)) && (*pMonitor >= (heat_min + db_offset)) && *pHeatState == 0) || ((*pMonitor <= (heat_max - db_offset)) && (*pMonitor >= (heat_min)) && *pHeatState != 0)){
			//((*pMonitor <= (heat_max + db_offset)) && (*pMonitor >= (heat_min + db_offset)) && *pHeatState == 0) || ((*pMonitor <= (heat_max - db_offset)) && (*pMonitor >= (heat_min - db_offset)) && *pHeatState != 0)

			double ramp, range;
			ramp = (*pMonitor > heating_setpoint0 ? heat_ramp_high : heat_ramp_low);
			range = (*pMonitor > heating_setpoint0 ? heat_range_high : heat_range_low);	

			double bid_setpoint;
			if (*pState == 0) // heating is off
			{
				bid_setpoint = heating_setpoint0 - db_offset;
			}
			else{
				bid_setpoint = heating_setpoint0 + db_offset;
			}

			if(*pMonitor != bid_setpoint){
				bid = proxy_avg + ( (fabs(proxy_stdev) < bid_offset) ? 0.0 : (*pMonitor - bid_setpoint) * ramp * (proxy_stdev) / fabs(range) );
			} else {

				bid = proxy_avg;
			}

			used_q = *pHeatingDemand;

		}
		// We might cool, if the price is right
		else if(((*pMonitor <= (cool_max - db_offset)) && (*pMonitor >= (cool_min)) && *pCoolState == 0) || ((*pMonitor <= (cool_max)) && (*pMonitor >= (cool_min + db_offset)) && *pCoolState != 0)){
			//((*pMonitor <= (cool_max - db_offset)) && (*pMonitor >= (cool_min - db_offset)) && *pCoolState == 0) || ((*pMonitor <= (cool_max + db_offset)) && (*pMonitor >= (cool_min + db_offset)) && *pCoolState == 1)
			double ramp, range;
			ramp = (*pMonitor > cooling_setpoint0 ? cool_ramp_high : cool_ramp_low);
			range = (*pMonitor > cooling_setpoint0 ? cool_range_high : cool_range_low);

			double bid_setpoint;
			if (*pState == 0) // cooling is off
			{
				bid_setpoint = cooling_setpoint0 + db_offset;
			}
			else{
				bid_setpoint = cooling_setpoint0 - db_offset;
			}

			if(*pMonitor != bid_setpoint){
				bid = proxy_avg + ( (fabs(proxy_stdev) < bid_offset) ? 0 : (*pMonitor - bid_setpoint) * ramp * (proxy_stdev) / fabs(range) );
			} else {
				
				bid = proxy_avg;
			}
			used_q = *pCoolingDemand;					

		}	
				
	} else {

		// We have to cool
		if(*pMonitor > cool_max){
			bid = proxy_price_cap;
			used_q = *pCoolingDemand;

		}
		// We have to heat
		else if(*pMonitor < heat_min){
			bid = proxy_price_cap;
			used_q = *pHeatingDemand;

		}
		// We're floating in between heating and cooling
		else if(*pMonitor > heat_max && *pMonitor < cool_min){
			bid = 0.0;
			used_q = 0.0;

		}
		// We might heat, if the price is right
		else if(*pMonitor <= heat_max && *pMonitor >= heat_min){
			double ramp, range;
			ramp = (*pMonitor > heating_setpoint0 ? heat_ramp_high : heat_ramp_low);
			range = (*pMonitor > heating_setpoint0 ? heat_range_high : heat_range_low);
			if(*pMonitor != heating_setpoint0){
				bid = proxy_avg + ( (fabs(proxy_stdev) < bid_offset) ? 0.0 : (*pMonitor - heating_setpoint0) * ramp * (proxy_stdev) / fabs(range) );
			} else {
				bid = proxy_avg;
			}
			used_q = *pHeatingDemand;

		}
		// We might cool, if the price is right
		else if(*pMonitor <= cool_max && *pMonitor >= cool_min){
			double ramp, range;
			ramp = (*pMonitor > cooling_setpoint0 ? cool_ramp_high : cool_ramp_low);
			range = (*pMonitor > cooling_setpoint0 ? cool_range_high : cool_range_low);
			if(*pMonitor != cooling_setpoint0){
				bid = proxy_avg + ( (fabs(proxy_stdev) < bid_offset) ? 0 : (*pMonitor - cooling_setpoint0) * ramp * (proxy_stdev) / fabs(range) );
			} else {
				bid = proxy_avg;
			}
			used_q = *pCoolingDemand;
		}
	}

	if(bid > proxy_price_cap)
		bid = proxy_price_cap;
	if(bid < -proxy_price_cap)
		bid = -proxy_price_cap;

	if(proxy_market_id != lastmkt_id){
		market_has_changed = true;
	}
	if(fabs(bid-last_p) > 0.0075){
		bid_has_changed = true;
	}
	if(fabs(used_q - fabs(last_q)) > 0.1){
		demand_has_changed = true;
	}

	// if our bid hasn't changed, don't re-send the same values.
	if(!(market_has_changed || bid_has_changed || demand_has_changed)){
		proxy_bid_ready = false;
		return TS_NEVER;
	} else {
		if(!no_bid){
			//gl_output("market %s change, dBid %g, dDemand (%g - %g)", (market_has_changed ? "DID" : "DID NOT"), fabs(bid-last_p), *pDemand,  fabs(last_q));
		}
	}

	residual = *pTotal;

	if(this->proxy_has_id){
		bid_id = (KEY)(lastmkt_id == proxy_market_id ? lastbid_id : -1);
	} else {
		bid_id = -1;
	}

	if(fabs(used_q) > 0.0 && no_bid != 1){
		last_q = used_q;
		last_p = bid;

		if(0 != strcmp(proxy_market_unit, "")){
			if(0 == gl_convert("kW", proxy_market_unit, &(last_q))){
				gl_error("unable to convert bid units from 'kW' to '%s'", proxy_market_unit.get_string());
				return TS_INVALID;
			}
		}
		
		if(bidmode != BM_PROXY){
			if (pState != 0 ) {
				//KEY bid_id = (KEY)(lastmkt_id == market->market_id ? lastbid_id : -1);
				last_pState = (*pState > 0 ? 1 : 0);
				lastbid_id = submit_bid_state(this->pMarket, OBJECTHDR(this), -last_q, last_p, (*pState > 0 ? 1 : 0), bid_id);
			}
			else {
				//KEY bid_id = (KEY)(lastmkt_id == market->market_id ? lastbid_id : -1);
				last_pState = BS_UNKNOWN;
				lastbid_id = submit_bid(this->pMarket, OBJECTHDR(this), -last_q, last_p, bid_id);
			}
		}
		else
		{
			char nstr[64];
			last_q = -last_q; // make negative for bid reasons
			gl_verbose("%s proxy-bidding %f for %f kW (st %i, bid_id %lli)", gl_name(hdr, nstr, 63), last_p, last_q, last_pState, bid_id);
			proxy_bid_ready = true; 
			proxy_bid_id = bid_id;
			/*
			if (last_pState != *pState)
			{
				KEY bid = (KEY)(lastmkt_id == market->market_id ? lastbid_id : -1);
				double my_bid = -market->pricecap;
				if (*pState != 0)
					my_bid = last_p;

				lastbid_id = submit_bid_state(this->pMarket, OBJECTHDR(this), -last_q, my_bid, (*pState > 0 ? 1 : 0), bid);
			} else {
				lastbid_id = -4; // zero quantity bid
			}
			*/
		}
		residual -= *pLoad;
	} else {
		last_p = 0;
		last_q = 0;
		gl_verbose("%s's is not bidding", hdr->name);
	}
	if(residual < -0.001)
		gl_warning("controller:%d: residual unresponsive load is negative! (%.1f kW)", hdr->id, residual);
	return TS_NEVER;
}

TIMESTAMP controller::sync(TIMESTAMP t0, TIMESTAMP t1){
	// some of these were in when ramp_control and double_ramp_control were monolithed in here.
	double bid = -1.0;
	int64 no_bid = 0; // flag gets set when the current temperature drops in between the the heating setpoint and cooling setpoint curves
	double demand = 0.0;
	double rampify = 0.0;
	extern double bid_offset;
	OBJECT *hdr = OBJECTHDR(this);
	TIMESTAMP rv = TS_NEVER;
	if(t0==t1)
		return rv;
	if(bidmode != BM_PROXY){
		// fill proxy values with current values
		proxy_market_id = market->market_id;
		proxy_clear_price = market->current_frame.clearing_price;
		proxy_avg = *pAvg;
		proxy_stdev = *pStd;
		
	} else { // == BM_PROXY
		if(proxystate == PS_READY){
			// assume that the proxy values are good and have been updated 'recently'
			;
		} else {
			// assume that the proxy-mode controller is NOT ready to calculate bids,
			//	and short-circuit all this
			no_bid_reason = 1;//The controller didn't bid because the proxystate wasn't PS_READY
			return TS_NEVER;
		}
	}
	
	/* short circuit if the state variable doesn't change during the specified interval */
	if(((bidmode != BM_PROXY) && (t1 < next_run) && (market->market_id == lastmkt_id)) || ((bidmode == BM_PROXY) && (t1 < next_run) && (proxy_market_id == lastmkt_id))){
		if(t1 <= next_run - bid_delay){
			if(bid_deadband_ends == TRUE && ((control_mode == CN_RAMP && last_setpoint != setpoint0) || (control_mode == CN_DOUBLE_RAMP && (lastHeatSetPoint != heating_setpoint0 || lastCoolSetPoint != cooling_setpoint0)))){
				//Only enabled if bid_setpoint_change is true i.e the user wants a rebidding 
				//when a set point change occurs in the middle of a market cycle
				;//continue
			} else if(no_bid_reason == 4){//The controller previously tried to rebid but it didn't because it hasn't recieved a bid id. Lets try again.
				if(t1 < bid_return){
					return bid_return;
				}
			} else {
				if (pState == 0){
				no_bid_reason = 2;//The controller didn't bid because it has no state property
				return next_run;
				} else if (*pState == last_pState){
					if(bidmode == BM_PROXY){
						proxy_bid_ready = false;
					}
					no_bid_reason = 3;//The controller didn't bid because it's state didn't change
					return next_run;
				}
			}
		} 
	}

	if(control_mode == CN_RAMP){
		rv = ramp_control(t0, t1);
	} else if (control_mode == CN_DOUBLE_RAMP){
		rv = double_ramp_control(t0, t1);
	} // TS_NEVER if successful, TS_INVALID otherwise

	switch(lastbid_id){
		case 0:
			// proxy, might just not have gotten a result yet.
			gl_verbose("controller bid resulted in an ambiguous result");
			bid_resp = 0;
			break;
		case -1:
			gl_error("there was a serious problem while calling submit_bid()");
			return TS_INVALID;
			break;
		case -2:
			gl_warning("the controller put in an early bid");
			bid_resp = 1;
			break;
		case -3:
			gl_warning("the controller put in a late bid");
			bid_resp = 2;
			break;
		case -4:
			gl_warning("the controller put in a zero quantity bid");
			bid_resp = 3;
			break;
		case -5:
			if(warn_warmup)
				gl_warning("the controller put in a bid during the warmup period");
				bid_resp = 4;
			break;
	}


	char timebuf[128];
	gl_printtime(t1,timebuf,127);
	//gl_verbose("controller:%i::sync(): bid $%f for %f kW at %s",hdr->id,last_p,last_q,timebuf);
	//return postsync(t0, t1);
	no_bid_reason = -1;// the controller sent a bid.
	return rv;
}

TIMESTAMP controller::postsync(TIMESTAMP t0, TIMESTAMP t1){
	TIMESTAMP rv = next_run - bid_delay;
	if(((lastHeatSetPoint != heating_setpoint0) || (lastCoolSetPoint != cooling_setpoint0)) && control_mode == CN_DOUBLE_RAMP)
	{
		lastHeatSetPoint = heating_setpoint0;
		lastCoolSetPoint = cooling_setpoint0;
	}
	if(last_setpoint != setpoint0 && control_mode == CN_RAMP){
		last_setpoint = setpoint0;
	}
	if(t1 < next_run-bid_delay){
		return rv;
	}

		// Determine the system_mode the HVAC is in
	if(resolve_mode == RM_SLIDING || resolve_mode == RM_DOMINANT){
		if(*pHeatState == 1 || *pAuxState == 1){
			thermostat_mode = TM_HEAT;
			if(last_mode == TM_OFF)
				time_off = TS_NEVER;
		} else if(*pCoolState == 1){
			thermostat_mode = TM_COOL;
			if(last_mode == TM_OFF)
				time_off = TS_NEVER;
		} else if(*pHeatState == 0 && *pAuxState == 0 && *pCoolState == 0){
			thermostat_mode = TM_OFF;
			if(previous_mode != TM_OFF)
				time_off = t1 + dtime_delay;
		} else {
			gl_error("The HVAC is in two or more modes at once. This is impossible");
			if(resolve_mode == RM_SLIDING)
				return TS_INVALID; // If the HVAC is in two modes at once then the sliding resolve mode will have conflicting state input so stop the simulation.
		}
	}

	if (t1 - next_run < bid_delay){
		rv = next_run;
	}

	if(next_run == t1){
		next_run += (TIMESTAMP)(this->period);
		rv = next_run - bid_delay;
	}

	return rv;
}

// triggered by notify_controller_proxy_update_time
// note to self, set proxy_update_time last, when updating proxy values from controller_nif
void controller::check_proxystate(char *value){
	// value will be the time string
	char32 namestr;
	int64 local_clear_time = 0;
	if(0 == strcmp(value, "NOW")){
		local_clear_time = gl_globalclock;
	} else {
		local_clear_time = strtoll(value, NULL, 10);
	}
	if(PS_READY == proxystate){
		// market sent an update
		;
		proxy_has_id = false;
		lastbid_id = 0;
	} else if(PS_INIT == proxystate){
		// controller initialization
		gl_output("period = %i, id = %lli, time = %s (%x), cap = %f", proxy_market_period, proxy_market_id, value, value, proxy_price_cap);
		if( (proxy_market_period > 0) &&
			(proxy_market_id > 0) &&
			(local_clear_time > 0) &&
			(proxy_price_cap > 0.0)){
				proxystate = PS_READY;
				proxy_market_id = -1;
				proxy_has_id = false;
				lastbid_id = 0;
				gl_name(OBJECTHDR(this), namestr, 31);
				gl_output("%s is ready at %lli", namestr.get_string(), local_clear_time);
		}
	}
}

void controller::update_proxy_bid(char *value){
	// value is the submit_bid() return value
	int bid_result = strtol(value, 0, 10);
	// if == bid_rv, things are okay, continue on.
	if(bid_result == proxy_bid_retval){
		proxy_has_id = true;
		if(bid_result != -1){
			lastbid_id = bid_result;
		} else {
			gl_warning("submit_bid() failed to properly submit the bid.");
		}
	} else {
		// if not 0, bid was not accepted, switch for reason
	}
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_controller(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(controller::oclass);
		if (*obj!=NULL)
		{
			controller *my = OBJECTDATA(*obj,controller);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(controller);
}

EXPORT int init_controller(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
		{
			return OBJECTDATA(obj,controller)->init(parent);
		}
		else
			return 0;
	}
	INIT_CATCHALL(controller);
}

EXPORT int isa_controller(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,controller)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_controller(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	controller *my = OBJECTDATA(obj,controller);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			//obj->clock = t1;
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock, t1);
			//obj->clock = t1;
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			obj->clock = t1;
			break;
		default:
			gl_error("invalid pass request (%d)", pass);
			return TS_INVALID;
			break;
		}
		return t2;
	}
	SYNC_CATCHALL(controller);
}

EXPORT int notify_controller_proxy_clear_time(OBJECT *obj, char *value){
	controller *my = OBJECTDATA(obj,controller);
//	gl_output("notify_controller_proxy(%s (%x)) called", value, value);
	// time has not been updated *yet*
	my->check_proxystate(value);
	return 1;
}

// this is called when the bid_id is set by the controller_nif, after receiving a bid_rsp message from the market_nif 
EXPORT int notify_controller_proxy_bid_id(OBJECT *obj, char *value){
	controller *my = OBJECTDATA(obj, controller);
	gl_output("notify_controller_proxy_bid_id(%s) called", value);
	// bid_result has not been updated *yet*
	my->update_proxy_bid(value);
	return 1;
}

EXPORT int notify_controller_proxy_bid_ready(OBJECT *obj, char *value){
//	char namestr[64];
//	gl_output("%s setting bid_ready to %s", gl_name(obj, namestr, 63), value);
	return 1;
}
// EOF

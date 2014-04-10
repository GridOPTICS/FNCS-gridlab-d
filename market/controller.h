/** $Id: controller.h 1182 2009-09-09 22:08:36Z mhauer $
	Copyright (C) 2009 Battelle Memorial Institute
	@file controller.h
	@addtogroup controller
	@ingroup market

 **/

#ifndef _controller_H
#define _controller_H

#include <stdarg.h>
#include "auction.h"
#include "gridlabd.h"
//#include "lock.h"

class controller {
public:
	controller(MODULE *);
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	static CLASS *oclass;

	void check_proxystate(char *);
	void update_proxy_bid(char *);
private:
	int init_direct(OBJECT *parent);
	int init_proxy(OBJECT *parent);
public:
	typedef enum {
		SM_NONE,
		SM_HOUSE_HEAT,
		SM_HOUSE_COOL,
		SM_HOUSE_PREHEAT,
		SM_HOUSE_PRECOOL,
		SM_WATERHEATER,
		SM_DOUBLE_RAMP,
	} SIMPLE_MODE;
	SIMPLE_MODE simplemode;
	
	typedef enum {
		BM_OFF,
		BM_ON,
		BM_PROXY,
	} BIDMODE;
	BIDMODE bidmode;

	typedef enum {
		CN_RAMP,
		CN_DOUBLE_RAMP,
	} CONTROLMODE;
	CONTROLMODE control_mode;
	
	typedef enum {
		RM_DEADBAND,
		RM_SLIDING,
	} RESOLVEMODE;
	RESOLVEMODE resolve_mode;

	typedef enum{
		TM_INVALID=0,
		TM_OFF=1,
		TM_HEAT=2,
		TM_COOL=3,
	} THERMOSTATMODE;
	THERMOSTATMODE thermostat_mode, last_mode, previous_mode;

	typedef enum {
		OU_OFF=0,
		OU_ON=1
	} OVERRIDE_USE;
	OVERRIDE_USE use_override;

	typedef enum {
		PS_NOTREADY=0,
		PS_INIT=1,
		PS_READY=2,
		PS_ERROR=3,
	} PROXY_STATE;
	PROXY_STATE proxystate;

	typedef enum {
		PR_NONE=0,
		PR_SENT=1,
		PR_SUCCESS=2,
		PR_FAIL_LATE=3,
		PR_FAIL_BOGUS=4,
		PR_FAIL_WARMUP=5,
		PR_FAIL_EARLY=6,
		PR_FAIL_ZEROQ=7,
		PR_FAIL_BADBIDNUM=8
	} PROXY_BID_RESULT;
	PROXY_BID_RESULT bid_result;

	double kT_L, kT_H;
	int32 bid_count;
	int32 last_bid_count;
	char target[33];
	char setpoint[33];
	char demand[33];
	char total[33];
	char load[33];
	char state[33];
	char32 avg_target;
	char32 std_target;
	OBJECT *pMarket;
	auction *market;
	KEY lastbid_id;
	KEY lastmkt_id;
	double last_p;
	double last_q;
	BIDDERSTATE bid_state;
	double set_temp;
	double set_temp_heat;
	double set_temp_cool;
	int may_run;

	// new stuff
	double ramp_low, ramp_high;
	double dPeriod;
	int64 period;
	double slider_setting;
	double slider_setting_heat;
	double slider_setting_cool;
	double range_low;
	double range_high;
	double heat_range_high;
	double heat_range_low;
	double heat_ramp_high;
	double heat_ramp_low;
	double cool_range_high;
	double cool_range_low;
	double cool_ramp_high;
	double cool_ramp_low;
	char32 heating_setpoint;
	char32 cooling_setpoint;
	char32 heating_demand;
	char32 cooling_demand;
	char32 heating_total;
	char32 cooling_total;
	char32 heating_load;
	char32 cooling_load;
	char32 heating_state;
	char32 cooling_state;
	char32 deadband;
	char32 re_override;
	enumeration last_pState;

	double setpoint0;
	double heating_setpoint0;
	double cooling_setpoint0;
	double sliding_time_delay;
	int bid_delay;
	bool warn_warmup;

	double last_setpoint;
	
	bool use_predictive_bidding;

	// proxy mode values
	bool proxy_bid_ready;
	OBJECT *proxy_object;
	TIMESTAMP proxy_update_time;
	double proxy_init_price;
	int32 proxy_market_period;
	char32 proxy_market_unit;
	int64 proxy_market_id;
	int64 proxy_bid_id;
	int32 proxy_bid_retval;
	TIMESTAMP proxy_clear_time;
	double proxy_clear_price;
	double proxy_marginal_frac;
	double proxy_price_cap;
	double proxy_avg;
	double proxy_stdev;
	int proxy_has_id;
	int proxy_delay;
	TIMESTAMP proxy_last_tx;
	int32 no_bid_reason;
	double bid_return_check;
	int32 bid_resp;
	TIMESTAMP bid_return;

private:
	TIMESTAMP ramp_control(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP double_ramp_control(TIMESTAMP t0, TIMESTAMP t1);

	TIMESTAMP next_run;
	TIMESTAMP init_time;
	double *pMonitor;
	double *pSetpoint;
	double *pDemand;
	double *pTotal;
	double *pLoad;
	double *pAvg;
	double *pStd;
	enumeration *pState;
	//enumeration last_pState;
	void cheat();
	void fetch(double **prop, char *name, OBJECT *parent);
	int dir, direction;
	double min, max;
	double T_lim, k_T;
	double heat_min, heat_max;
	double cool_min, cool_max;
	double *pDeadband;
	double *pHeatingSetpoint;
	double *pCoolingSetpoint;
	double *pHeatingDemand;
	double *pCoolingDemand;
	double *pHeatingTotal;
	double *pCoolingTotal;
	double *pHeatingLoad;
	double *pCoolingLoad;
	enumeration *pHeatingState;
	enumeration *pCoolingState;
	double *pAuxState;
	double *pHeatState;
	double *pCoolState;
	char32 heat_state;
	char32 cool_state;
	char32 aux_state;
	int64 dtime_delay;
	TIMESTAMP time_off;
	bool use_market_period;
	double lastHeatSetPoint;
	double lastCoolSetPoint;

	enumeration *pOverride;
};

#endif // _controller_H

// EOF

/** $Id: sectionalizer.h,v 2.2 2011/01/11 23:00:00 d3x593 Exp $
	Copyright (C) 2011 Battelle Memorial Institute
	@file sectionalizer.h
 @{
 **/

#ifndef SECTIONALIZER_H
#define SECTIONALIZER_H

#include "powerflow.h"
#include "switch_object.h"

class sectionalizer : public switch_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	sectionalizer(MODULE *mod);
	inline sectionalizer(CLASS *cl=oclass):switch_object(cl){};
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
};

EXPORT double change_sectionalizer_state(OBJECT *thisobj, unsigned char phase_change, bool state);
EXPORT int sectionalizer_reliability_operation(OBJECT *thisobj, unsigned char desired_phases);
EXPORT int sectionalizer_fault_updates(OBJECT *thisobj, unsigned char restoration_phases);

#endif // SECTIONALIZER_H
/**@}**/
/*
    Copyright (c) 2013, <copyright holder> <email>
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "transmissioncom.h"


CLASS *transmissioncom::oclass = NULL;

transmissioncom *transmissioncom::def=NULL;

transmissioncom::transmissioncom(MODULE* module)
{
  memset(this, 0, sizeof(transmissioncom));
  if(oclass==NULL){
    oclass=gl_register_class(module,"transmissioncom",sizeof(transmissioncom),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
    if (oclass==NULL)
	  GL_THROW("unable to register object class implemented by %s",__FILE__);
  }
  if (gl_publish_variable(oclass,
    PT_char256,"connected_bus",PADDR(connectedBus),
    PT_char32, "parent_voltage_property", PADDR(voltage_property),
    PT_char32, "parent_power_property", PADDR(power_property),
			  NULL)<1) 
	GL_THROW("unable to publish properties in %s",__FILE__);
	
  def=this;
}

int transmissioncom::create()
{
  memcpy(this,def,sizeof(transmissioncom));
  P = 0;
  Q = 0;
  last_power = complex(0,0);
  return 1;
}

int transmissioncom::isa(char *classname)
{
	if(classname != 0)
		return (0 == strcmp(classname,"transmissioncom"));
	else
		return 0;
}

complex * transmissioncom::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}

bool *transmissioncom::get_bool(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_bool)
		return NULL;
	return (bool*)GETADDR(obj,p);
}

double *transmissioncom::get_double(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_double)
		return NULL;
	return (double*)GETADDR(obj,p);
}

int transmissioncom::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	

	// input validation checks
	// * parent is a controller
	if(0 == parent){
		gl_error("init(): no parent object");
		return 0;
	}

	if(!gl_object_isa(parent, "substation", "powerflow") && !gl_object_isa(parent, "meter", "powerflow")){
		gl_error("init(): parent is not a powerflow:substation or a powerflow:meter.");
		return 0;
	}

	//TODO: datafromgld=gl_get_property.....
	datafromgld = get_complex(parent, power_property);
	datatogld = get_complex(parent, voltage_property);
	if(gl_object_isa(parent, "substation", "powerflow")){
		powerdiff = get_double(parent,"power_convergence_value");
	} else {
		default_powerdiff = 1; //default is 1 VA
		powerdiff = &default_powerdiff;
	}
	myinterface=Integrator::getCommInterface(hdr->name);
	printf("MY INTEFRACE is mull %d\n",myinterface==NULL);
}

TIMESTAMP transmissioncom::presync(TIMESTAMP t0, TIMESTAMP t1)
{
  complex temp = complex(0,0);
  complex last_voltage = complex(0,0);

	
	if(t0 < t1 && myinterface->hasMoreMessages()){
		last_voltage = *datatogld;
		temp = getFromTransmissionSolver();
		if(last_voltage.Mag() != 0.0){
			complex volt_diff = last_voltage - temp;
			double perc_diff = volt_diff.Mag()/last_voltage.Mag();
			if(perc_diff >= 0.5){
				if(last_voltage.Im() >= 0.0){
					gl_verbose("previous voltage: %lf+j%lf V.\n",last_voltage.Re(),last_voltage.Im());
				} else {
					gl_verbose("previous voltage: %lf+j%lf V.\n",last_voltage.Re(),-1.0*last_voltage.Im());
				}
				if(temp.Im() >= 0.0){
					gl_verbose("current voltage: %lf+j%lf V.\n",temp.Re(),temp.Im());
				} else {
					gl_verbose("current voltage: %lf+j%lf V.\n",temp.Re(),-1.0*temp.Im());
				}
				gl_verbose("voltage changed more that 50 percent. Something isn't right in transmission. Using previous voltage.\n");
			} else {
				*datatogld = temp;
			}
		} else if(temp.Mag() > 0.0){
			*datatogld = temp;
		}
	}
	return TS_NEVER;

}

complex transmissioncom::getFromTransmissionSolver()
{
  	complex toReturn(0,0);
	Message *myMesg=myinterface->getNextInboxMessage();
	const uint8_t* data=myMesg->getData();
	double realPart;
	double imagineryPart;
	memcpy(&realPart,data,sizeof(double));
	memcpy(&imagineryPart,&data[sizeof(double)],sizeof(double));
	toReturn.SetReal(realPart);
	toReturn.SetImag(imagineryPart);
	delete myMesg;

  	return toReturn;
}

void transmissioncom::sendToTransmissionSolver(complex value)
{
  uint8_t data[sizeof(double)*2];
  double realPart=value.Re();
  double imagineryPart=value.Im();
  memcpy(&data,&realPart,sizeof(double));
  memcpy(&data[sizeof(double)],&imagineryPart,sizeof(double));
  
  OBJECT *hdr = OBJECTHDR(this);
  Message *msg=new Message(hdr->name,this->connectedBus,gl_globalclock,(uint8_t*)data,sizeof(double)*2);
  msg->setDelayThroughComm(false);
  myinterface->send(msg);
}



TIMESTAMP transmissioncom:: postsync(TIMESTAMP t0,TIMESTAMP t1)
{
	//TODO: read the distribution load from parent and if different send message to transmission
	complex temp = complex(0,0);
		temp.SetReal(datafromgld->Re()/1000000);
		temp.SetImag(datafromgld->Im()/1000000);
		double d=temp.Mag() - last_power.Mag();
		d= d < 0 ? -1*d : d;
		if(d >= (*powerdiff/1000000)){// powerdiff by default is 1 VA
			last_power = temp;
			sendToTransmissionSolver(temp);
		}
	return TS_NEVER;
}



TIMESTAMP transmissioncom::commit(TIMESTAMP t0, TIMESTAMP t1)
{
  return TS_NEVER;
}

EXPORT int init_transmissioncom(OBJECT *obj)
{
	transmissioncom *my = OBJECTDATA(obj,transmissioncom);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		gl_error("%s::%s.init(OBJECT *obj={name='%s', id=%d}): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, msg);
		return 0;
	}
}

EXPORT int create_transmissioncom(OBJECT **obj, OBJECT *parent)
{
	//printf("HERE size %d\n",transmissioncom::oclass->size);
	*obj = gl_create_object(transmissioncom::oclass);
	if (*obj!=NULL)
	{
		transmissioncom *my = OBJECTDATA(*obj,transmissioncom);
		gl_set_parent(*obj,parent);
		try {
			my->create();
		}
		catch (char *msg)
		{
			gl_error("%s::%s.create(OBJECT **obj={name='%s', id=%d},...): %s", (*obj)->oclass->module->name, (*obj)->oclass->name, (*obj)->name, (*obj)->id, msg);
			return 0;
		}
		return 1;
	}
	return 0;
}

EXPORT TIMESTAMP sync_transmissioncom(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass){
	transmissioncom *my = OBJECTDATA(obj,transmissioncom);
	try {
		TIMESTAMP t2 = TS_NEVER;
		if(pass == PC_BOTTOMUP){
			t2 = my->sync(obj->clock, t1);
		} else if(pass == PC_POSTTOPDOWN){
			t2 = my->postsync(obj->clock, t1);
		} else if(pass == PC_PRETOPDOWN){
			t2 = my->presync(obj->clock, t1);
		}
		obj->clock = t1;
		return t2;
	}
	catch (const char *msg)
	{
		DATETIME dt;
		char ts[64];
		gl_localtime(t1,&dt);
		gl_strtime(&dt,ts,sizeof(ts));
		gl_error("%s::%s.init(OBJECT **obj={name='%s', id=%d},TIMESTAMP t1='%s'): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, ts, msg);
		return 0;
	}
}

EXPORT int isa_transmissioncom(OBJECT *obj, char *classname)
{	
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,transmissioncom)->isa(classname);
	} else {
		return 0;
	}
}

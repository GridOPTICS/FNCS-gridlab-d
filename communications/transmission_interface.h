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


#ifndef TRANSMISSIONCOM_H
#define TRANSMISSIONCOM_H
#include "gridlabd.h"
#include "objectcomminterface.h"

using namespace sim_comm;

class transmissioncom : public gld_object
{
private:
      ObjectCommInterface *myinterface;
      complex *datatogld;   // This is a pointer to the property you want to send to the transmission solver
      complex *datafromgld; // This is a pointer to the property you want to put data from the transmission solver
      complex last_power;   //this is the value we should be writing the information from the transmission solver
      double P;
      double Q;
      double *powerdiff;
      bool *read_power;
      double default_powerdiff;
      complex datafromsolver; //this is the value we get from transmisison solver.
      complex getFromTransmissionSolver();
      void sendToTransmissionSolver(complex value);
      
public:
	static CLASS *oclass;
	static transmissioncom *def;
public:
	char32 voltage_property;
	char32 power_property;
	char256 connectedBus;
	transmissioncom(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	inline TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1) { return TS_NEVER; };
    TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP commit(TIMESTAMP t0, TIMESTAMP t1);
	complex *get_complex(OBJECT *obj, char *name);
	bool *get_bool(OBJECT *obj, char *name);
	double *get_double(OBJECT *obj, char *name);
};
  

#endif // TRANSMISSIONCOM_H

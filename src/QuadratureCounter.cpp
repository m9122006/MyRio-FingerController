/*
 * QuadratureCounter.cpp
 *
 *  Created on: Nov 15, 2017
 *      Author: Janco
 */

#include "RotaryEncoder.h"
#include <stdio.h>
#include <time.h>
#include "Encoder.h"
#include "MyRio.h"

extern NiFpga_Session myrio_session;


QuadratureCounterEncoder::QuadratureCounterEncoder(const Encoder_Config *encoder_config){
	this->steps = 0;
	this->direction = "";
	this->encoder = encoder_config->encoder;
	uint8_t selectReg;

	/*
	 * Encoder inputs are on pins shared with other onboard devices. To input
	 * from a physical pin, select the encoder on the appropriate SELECT
	 * register.
	 *
	 * Read the value of the SYSSELECTB register.
	 */
	NiFpga_ReadU8(myrio_session, encoder_config->indicator, &selectReg);

	/*
	 * Set bit 5 of the SYSSELECTB register to enable ENCB functionality.
	 * The functionality of these bits is specified in the documentation.
	 */
	selectReg = selectReg | (1 << 5);

	/*
	 * Write the updated value of the SYSSELECTB register.
	 */
	NiFpga_WriteU8(myrio_session, encoder_config->indicator, selectReg);

	/*
	 * Enable the encoder and configure to read step and direction signals.
	 */

	Encoder_Configure(&this->encoder, Encoder_Enable | Encoder_SignalMode, Encoder_Enabled | Encoder_StepDirection);
}


uint32_t QuadratureCounterEncoder::readSteps(){
	steps = Encoder_Counter(&this->encoder);
	if ((Encoder_Status(&this->encoder) & Encoder_StDirection) == Encoder_Incrementing){
		direction = "incrementing";
	}else{
		direction = "decrementing";
	}
	return steps;
}


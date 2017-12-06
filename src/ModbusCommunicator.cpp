/*
 * I2CCommunicator.cpp
 *
 *  Created on: Nov 8, 2017
 *      Author: Project team C2
 */

#include "ModbusCommunicator.h"
#include "string.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "DIO.h"
#include <unistd.h>
#include <pthread.h>

int convert_slice(const char *s, size_t a, size_t b) {
	char tmp[b - a];
	strncpy(tmp, s+a, b-a);
    return (int)strtol(tmp, NULL, 16);
}

ModbusCommunicator::ModbusCommunicator(FingerController *fingerController){
    this->fingerController = fingerController;

	this->writePin = {DIOA_70DIR, DIOA_70OUT, DIOA_70IN, 0};

	holdingRegisters[0] = (int*)fingerController->motor_controller1->encoder->steps,
	holdingRegisters[1] = (int*)fingerController->motor_controller2->encoder->steps,
	holdingRegisters[2] = (int*)fingerController->motor_controller3->encoder->steps,


    /*;
     * Initialize the UART port structure.
     */
    uart.name = "ASRL1::INSTR";
    uart.defaultRM = 0;
    uart.session = 0;

    /*
     * Opens a UART session.
     */
    Uart_Open(&uart, BaudRate, DataBit, Uart_StopBits1_0, Uart_ParityNone);
}

void ModbusCommunicator::run(){
	uint8_t readData = 0;
    int32_t status = 0;

    /*
     * Reads data from a UART port.
     */
    printf("Start reading modbus messages \n");
    char message[255] = {0};
    int position = 0;
    enableRX();

    printf("uart: %s \n",  this->uart.name);
    for(;;){
        status = Uart_Read(&uart, &readData, 1);
        if (status != VI_ERROR_TMO){
            if(readData == ':'){		//New message, discard old message
            	memset(message, 0, sizeof message );
            	position = 0;
            }else if(readData == '\n'){	//Message ended.
            	message[position] = '\0';
            	printf("Message received!: %s \n\n", message);
            	parseMessage(message, position);
            }else if(readData != '\r'){
            	message[position] = readData;
            	position++;
            }
        }
    }
}

void ModbusCommunicator::parseMessage(char *message, int length){
	short lrc_calc = ModbusCommunicator::calculateLRC(message, 0, length-2);

	int adress = convert_slice(message, 0, 2);
	int function = convert_slice(message, 2, 4);
	int lrc = convert_slice(message, length-2, length);


	printf("Address: %d Function code: %d LRC: %d Calculated LRC %d \n", adress, function, lrc, lrc_calc);
	if(lrc == lrc_calc && adress == SlaveAdress){ //Message located to us
		if(function == 3){
			readHoldingRegister(message, length);
		}
	}
}

void ModbusCommunicator::readHoldingRegister(char *message, int length){
	int startAdress = convert_slice(message, 4, 8);
	int registerQuantity = convert_slice(message, 8, 12);

	printf("Reading holding registor from %d and a number of %d \n", startAdress, registerQuantity);

	int dataLength = 4 + registerQuantity * 4;
	char data[dataLength];
	data[0] = '0';								//Function code
	data[1] = '3';								//Function code
    sprintf(&data[2], "%02x", registerQuantity * 2);	//Byte count
    for(int i = 0; i < registerQuantity; i++){
        sprintf(&data[4 + i * 4], "%04x", holdingRegisters[startAdress + i]);				// Register value
    }

    sendData(data, dataLength);

}

void ModbusCommunicator::sendData(char *data, int length){
	int msgLength = length + 7;
	char message[msgLength];

	message[0] = ':';							//Start byte
    sprintf(&message[1], "%02x", SlaveAdress);	//Slave Address
    sprintf(&message[3], "%s", data);			//Data
    sprintf(&message[3+length], "%02x", calculateLRC(message, 1, length+3));	//LRC
    message[3+length+2] = '\r';					//End char
    message[3+length+3] = '\n';					//End char

    printf("Sending message: %s \n", message);

    uint8_t message_uint8[msgLength];

    for(int i = 0; i < msgLength; i++){
    	message_uint8[i] = message[i];
    }

    enableTX();

    Uart_Write(&uart, message_uint8, msgLength);

	enableRX();
}

short ModbusCommunicator::calculateLRC(char *message, int start, int end){
	int lrc = 0;
	int fctr = 16;

	char buf[0];
	for (int i = start; i < end; i++){
		buf[0] = message[i];
	    int val = strtol(buf, NULL, 16);

		lrc += val * fctr;

        if (fctr == 16) fctr = 1;
        else            fctr = 16;
	}
	lrc = (~(lrc & 0xff) + 1) & 0xFF;
	return lrc;
}

void ModbusCommunicator::enableRX(){
    Dio_WriteBit(&writePin, NiFpga_False);
}

void ModbusCommunicator::enableTX(){
    Dio_WriteBit(&writePin, NiFpga_True);
}



/* ************************************************************************** */
/** Radio Communication to Base Station
/* ************************************************************************** */
#define _SUPPRESS_PLIB_WARNING // removes outdated plib warning
#define _DISABLE_OPENADC10_CONFIGPORT_WARNING

#include <plib.h>
#include <stdio.h>
#include "tft_master.h"
#include "sensors.h"
#include "navigation_helper.h"

uint32_t clock_rt = 40000000;

void initRadio(void) {
    // define setup Configuration 1 for OpenUARTx
		// Module Enable 
		// Work in IDLE mode 
		// Communication through usual pins 
		// Disable wake-up 
		// Loop back disabled 
		// Input to Capture module from ICx pin 
		// no parity 8 bit 
		// 1 stop bit 
		// IRDA encoder and decoder disabled 
		// CTS and RTS pins are disabled 
		// UxRX idle state is '1' 
		// 16x baud clock - normal speed
	#define config1 	UART_EN | UART_IDLE_CON | UART_RX_TX | UART_DIS_WAKE | UART_DIS_LOOPBACK | UART_DIS_ABAUD | UART_NO_PAR_8BIT | UART_1STOPBIT | UART_IRDA_DIS | UART_DIS_BCLK_CTS_RTS| UART_NORMAL_RX | UART_BRGH_SIXTEEN
	
	// define setup Configuration 2 for OpenUARTx
		// IrDA encoded UxTX idle state is '0'
		// Enable UxRX pin
		// Enable UxTX pin
		// Interrupt on transfer of every character to TSR 
		// Interrupt on every char received
		// Disable 9-bit address detect
		// Rx Buffer Over run status bit clear
	#define config2		UART_TX_PIN_LOW | UART_RX_ENABLE | UART_TX_ENABLE | UART_INT_TX | UART_INT_RX_CHAR | UART_ADR_DETECT_DIS | UART_RX_OVERRUN_CLEAR

	// Open UART2 with config1 and config2
	OpenUART2( config1, config2, clock_rt/16/9600-1);	// calculate actual BAUD generate value.
		
	// Configure UART2 RX Interrupt with priority 2
    IFS1bits.U2EIF = 0;
    IFS1bits.U2RXIF = 0;
    IFS1bits.U2TXIF = 0;
    IPC9bits.U2IP = 2;
    IPC9bits.U2IS = 2;
    IEC1bits.U2RXIE = 1;
    IEC1bits.U2TXIE = 0;
    
    PPSInput (2, U2RX, RPA1); //Assign U2RX
    PPSOutput(4, RPA3, U2TX); //Assign U2TX
}

void transmitString(char *data) {
    putsUART2(data);
}

void printData(void) {
    char buffer[80];
    transmitString("----------NAVIGATION----------\n");
    
    sprintf(buffer, "Y position: %.10f\n", sensorData->y);
    transmitString(buffer);
    sprintf(buffer, "X position: %.10f\n", sensorData->x);
    transmitString(buffer);
    
    sprintf(buffer, "latitude: %.10f\n", sensorData->lat);
    transmitString(buffer);
    sprintf(buffer, "longitude: %.10f\n", sensorData->longi);
    transmitString(buffer);
    
    sprintf(buffer, "Wind w.r.t North: %f\n", sensorData->wind_dir);
    transmitString(buffer);
    
    sprintf(buffer, "Boat direction: %f\n", sensorData->boat_direction);
    transmitString(buffer);
    
    sprintf(buffer, "Distance to Waypoint: %f\n", navData->distToWaypoint);
    transmitString(buffer);
    sprintf(buffer, "Angle to Waypoint: %f\n", navData->angleToWaypoint);
    transmitString(buffer);
    
    sprintf(buffer, "Roll: %f\n", sensorData->roll);
    transmitString(buffer);
    sprintf(buffer, "Pitch: %f\n", sensorData->pitch);
    transmitString(buffer);
    
    sprintf(buffer, "Next Waypoint #:%d\n", navData->currentWaypoint);
    transmitString(buffer);
    sprintf(buffer, "Next Waypoint X: %.10f\n", waypoints[navData->currentWaypoint].x);
    transmitString(buffer);
    sprintf(buffer, "Next Waypoint Y: %.10f\n", waypoints[navData->currentWaypoint].y);
    transmitString(buffer);
    
    sprintf(buffer, "Sail angle: %f\n", sensorData->sailAngleBoat);
    transmitString(buffer);
    sprintf(buffer, "Tail angle: %f\n", sensorData->tailAngleBoat);
    transmitString(buffer);
}

void printHitWaypointData(void) {
  transmitString("\n");
  transmitString("\n");
  transmitString("\n");
  transmitString("----------\n");
  transmitString("REACHED WAYPOINT!\n");
  transmitString("----------\n");
  transmitString("\n");
  transmitString("\n");
  transmitString("\n");
}

// UART 2 interrupt handler
// it is set at priority level 2
void __ISR(_UART2_VECTOR, IPL2AUTO) IntUart2Handler(void)
{
	// Is this an RX interrupt?
	if(IFS1bits.U2RXIF)
	{
        // TODO read data in a meaningful way
		while (DataRdyUART2()) {
            char data = ReadUART2();
        }
        // Clear the RX interrupt Flag
	    IFS1bits.U2RXIF = 0;
	}

	// We don't care about TX interrupt
	if (IFS1bits.U2TXIF)
	{
		IFS1bits.U2TXIF = 0;
	}
}
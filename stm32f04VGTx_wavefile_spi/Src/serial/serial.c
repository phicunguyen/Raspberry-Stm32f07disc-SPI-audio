/*
 * serial.c
 *
 *  Created on: Dec 7, 2019
 *      Author: pnguyen
 */

/*
 * Convert ascii byte to on hex byte
 */

#include "serial.h"
#include "../audioplay.h"

#define USE_GPIO__INT

/* Private define ------------------------------------------------------------*/
static struct serial_data_t serial_data;
/* Private functions ------------------------------------------------------------*/
static uint32_t timeCap[256];
static uint32_t timeCapCnt=0;
static uint8_t dataLookup=0x00;

/*
 * packet opcode
 */
static uint16_t s_get_opcode(void) {
	return (uint16_t)((uint16_t)serial_data.buf[0] | (serial_data.buf[1] <<8));
}

/*
 * parse the packet
 */
void packet_audio_play(uint8_t val) {
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
}

/*
 * parse the packet
 */
static void parse_packet(void) {
	switch(s_get_opcode()) {
		case SERIAL_PACKET__LED:
			break;
		case SERIAL_PACKET__FLASH_WRITE:
			break;
		case SERIAL_PACKET__AUDIO_PLAY:
			AudioOut_Play(&serial_data.buf[2], serial_data.len-2);
			break;
		case SERIAL_PACKET__AUDIO_STOP:
			break;
		default:
			break;
	}
}

/*
 * Packet start with '[' and end with ']'
 * */
void vcp_store_data(uint8_t *ptr, uint32_t len) {
	for (uint32_t i=0; i < len; i++){
		switch (ptr[i]) {
		case '[': //begin packet receiving
			serial_data.len = 0;
			serial_data.valid=true;
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
			timeCap[timeCapCnt] = HAL_GetTick();
			continue;
		case ']': //endpacket receiving
			serial_data.valid=false;
			timeCap[timeCapCnt] = HAL_GetTick() - timeCap[timeCapCnt];
			timeCapCnt = (timeCapCnt + 1) % 256;
			parse_packet();
			continue;
		default: //strore the data in the buffer
			if (serial_data.valid) {
				if (dataLookup == 0x5A || dataLookup == 0x5C) {
					serial_data.buf[serial_data.len] += ptr[i];
					dataLookup = 0x00;
				} else {
					serial_data.buf[serial_data.len++] = ptr[i];
					dataLookup = ptr[i];
				}
			}
			continue;
		}
	}
}



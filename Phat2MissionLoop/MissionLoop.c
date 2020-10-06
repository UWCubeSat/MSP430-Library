/*
 * MissionLoop.c
 *
 *  Created on: Feb 27, 2020
 *      Author: Logan Ince
 */
 
 #include <stdio.h>
 
 /*TODO
  * get_altitude
  * ppt_stop
  * ppt_fire
  * langmuir_read
  */
 
 int main(void) {
 
 	uint32_t altitude = //get_altitude
 	uint32_t start_altitude = 60000; // 60,000 ft
 	uint32_t end_altitude = 90000; // 90,000 ft
 	
 	// Wait until altitude reaches start altitude
 	while(altitude < start_altitude) {
 		altitude = //get_altitude
 	}
 	
 	// PPT begins fire at 60,000 ft stops firing at 90,000 ft or when altitude begins to decrease
 	// ppt_fire
 	while(altitude >= start_altitude) {
 		uint32_t new_altitude = //get_altitude
 		uint32_t delta_altitude = new_altitude - altitude;
 		altitude = new_altitude
 		if(altitude >= end_altitude || delta_altitude < 0) {
 		    //ppt_stop
 			break();
 		}
 		// langmuir_read
 		wait(9000); // Wait for 9 seconds, until next pulse
 	}
 	return 1;
}
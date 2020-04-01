/*
 * Driver_Core.h
 *
 *  Created on: Apr 9, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef DRIVER_CORE_H_
#define DRIVER_CORE_H_



/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/




/********************************************************************************/
/*																				*/
/*								Definitions	 									*/
/*																				*/
/********************************************************************************/




/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/

int __VolumeIO_Handler(struct DMS_Volume *volume, struct request *kreq);
int __VolumeIO_Commiter(void *data, int result);

extern int Default_Read_Policy(void *data, int nr_dn_locs);

#endif /* DRIVER_CORE_H_ */

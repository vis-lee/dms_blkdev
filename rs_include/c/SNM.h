/*
 * SNM.h
 *
 *  Created on: Oct 19, 2010
 *      Author: root
 */

#include "../global/c_session.h"

//mock port and switch to test
typedef struct ConnectivityInfoStruct
{
	int type_src;
	char ip_addr_src[128];
	char mac_addr_src[128];
	long port_src;
	int type_dest;
	char ip_addr_dest[128];
	char mac_addr_dest[128];
	long port_dest;
	struct ConnectivityInfoStruct* connectivityInfoNext;
}connectivityInfo;

//int createSwitchLinks(char *switch_ip, connectivityInfo *linkData);
//connectivityInfo* readSwitchLinks(char *switch_ip);
//connectivityInfo* readAllLinks();
//int deleteSwitchLinks(char *switch_ip);
//int deleteAllLinks();

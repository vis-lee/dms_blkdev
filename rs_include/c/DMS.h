/*
 * DMS.h
 *
 *  Created on: 2010/6/17
 *      Author: root
 */

#ifndef DMS_H_
#define DMS_H_

#include "../global/c_session.h"
#include "../global/api_version.h"

typedef struct VolumeInfo{
		char	volumeId[128];
		char	name[128];
		long	volume_size;
		int	    is_attached;
		int 	is_systemVolume;
		char	create_time[128];
		char	instId[128];
		struct VolumeInfo* volumeInfoNext;
}volumeInfo;

typedef struct Nic
{
	int	sequence_num;
	char	IPv4[128];
	char	mac_addr[128];
	int	IS_BMC;
	int	status;
	struct Nic* next;
}NicStruct;

typedef struct IpInfo{
		char	addr[15];
		struct IpInfo* next;
}ipInfo;

// SET volume inst_id and is_attach is true
int attachVolume(c_session *, const char * instUUID, const char * volUUID);
// is_attach is false
int detachVolume(c_session *, const char * instUUID, const char * volUUID);
volumeInfo* queryVolumebyUUID(c_session *, const char * volUUID);

// 11/10 get the whole ip addresses of datanodes
// return value : -1 is failed, other numbers mean the number of datanode
// int getDatanodeIP(c_session *, ipInfo *);
int getDatanodeIP(c_session *, ipInfo **);

#endif /* DMS_H_ */

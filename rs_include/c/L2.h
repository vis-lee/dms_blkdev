/*
 * L2.h
 *
 *  Created on: Sep 06, 2010
 *      Author: Bonda
 */

#include "../global/c_session.h"
#include "../global/api_version.h"

typedef struct Nic
{
	int	sequence_num;
	char	IPv4[128];
	char	mac_addr[128];
	int	IS_BMC;
	int	status;
	struct Nic* next;
}NicStruct;

typedef struct PNodeC
{
	long	node_id;
	char	host_name[128];
	NicStruct* nic;
	struct PNodeC* next;
}PNodeStruct;

typedef struct vmInfo
{
	char inst_ip_addr[128];
	char inst_mac_addr[128];
	char pm_ip[128];
	long node_id;
	long vdc_id;
	struct vmInfo* next;
}vmInfoStruct;

typedef struct connectivityInfo
{
	int type_src;
	char ip_addr_src[128];
	char mac_addr_src[128];
	long port_src;
	int type_dest;
	char ip_addr_dest[128];
	char mac_addr_dest[128];
	long port_dest;
	struct connectivityInfo* next;
}connectivityInfoStruct;

PNodeStruct* getAllNode( c_session* session);
vmInfoStruct* getAllInstance( c_session* session);	// not sure the returns of  getAllInstance and getVMinfoByVMID are the same, so separate to two structs
vmInfoStruct* getVMinfoByVMID( c_session* session, long vm_id);
//connectivityInfoStruct* getConnectivityByNodeIPandPort( c_session* session, char* IP, long port);

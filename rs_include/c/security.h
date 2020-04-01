/*
 * security.h
 *
 *  Created on: 2010/5/28
 *      Author: root
 */

#ifndef SECURITY_H_
#define SECURITY_H_

#include "../global/c_session.h"
#include "../global/api_version.h"

#define 	PRM		0x0001
#define		SNM		0x0002
#define		VDCM	0x0004
#define		RPM		0x0008
#define		RS		0x0010
#define		APIS	0x0020
#define		LOG		0x0040
#define		DMS		0x0080
#define		SLB		0x0100
//#define     RAS     0x0200
//#define     NTP     0x0400
#define     NRS     0x0800
#define     DS      0x1000
#define     SECS    0x2000
#define     DSS     0x4000
//#define     HIDS	0x8000
#define     CN      0x0200
#define     STORAGE 0X0400

typedef struct InstanceInfo
{
	long inst_id;
	long vc_id;
	char ip_address[100];
	char mac_address[100];
	char pub_mac_address[100];
	struct InstanceInfo* next;
}instInfo;

typedef struct ServiceNodeInfo
{
	long node_id;
	char service_name[100];
	char ip_address[100];
	struct ServiceNodeInfo* next;
}serviceNodeInfo;

typedef struct VDCInfo0
{
	long vdc_id;	//0
	long vc_id;		//0
	serviceNodeInfo* node;
}VDC0;

typedef struct VCInfo {
	long vc_id;
	instInfo* inst;
	struct VCInfo* next;
}vcInfo;

typedef struct VDCInfo
{
	VDC0* vdc0;
	long vdc_id;
	vcInfo* vc;
	struct VDCInfo* next;
}vdcInfo;

typedef struct IPList
{
	char node_ip[100];
	struct IPList *next;
}ipList;

typedef struct FWRule {
	char source_ip[100];
	int to_Port;
	int from_Port;
	char protocol_name[100];
	struct FWRule *next;
}fwRule;

typedef struct VCList
{
	char vc_ip[100];
	long fw_id;
	struct VCList *next;
}vcList;

typedef struct VMPWInfo
{
	char vdc_account[128];
	char vdc_password[128];
	char vc_account[128];
	char vc_password[128];
	char inst_account[128];
	char inst_password[128];
}vmPWInfo;

//get All Virtual Cluster information, include ip and fw id
vcList* getAllVCList(c_session* session);

//get node list (which include node ip address) by VDCUUID
ipList* getBroadcastIPListbyVDCUUID(c_session* session, char* vdcUUID);

//get VDC isolation table for Security Kernel by passing VDCUUID
vcInfo* getIpFilterTablebyVdcUUID(c_session* session, char* vdcUUID);
vcInfo* getIpFilterTablebyVdcID(c_session *session, long vdc_id);

//get FW rules by fw policy id
fwRule* getFWPolicyRulebyFWID(c_session* session, long fwID);

//get VC information from each node
vcList* getVCFWPolicyListbyNodeIP(c_session* session, char* nodeIP);

// ADD NEW API TO GET INST LIST
instInfo* getInstanceListbyNodeIP(c_session* session, char* nodeIP);

//get all vdc information, include vdc_id, vdc0, vc, and inst struct data
vdcInfo* getAllVDCInfo(c_session* session, long vdc0_option);	// vdc0_option is a bit vector to represent service node

//get service node information: node_id, service_name, ip address
serviceNodeInfo* getServiceNodeInfo( c_session* session, long serviceNodeOption);

//get node list (which include node ip address) by VCUUID
ipList* getBroadcastIPListbyVCIP(c_session* session, char* VCIP);

vdcInfo* getIpFilterTablebyNodeIP(c_session* session, char* IP, long vdc0_option);	// vdc0_option is a bit vector to represent service node

vmPWInfo* getVMPWInfobyVMMAC(c_session* session, char* vmMac);
#endif /* SECURITY_H_ */

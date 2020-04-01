/*
 * SLB.h
 *
 *  Created on: 2010/6/20
 *      Author: root
 */

#ifndef SLB_H_
#define SLB_H_

#include "../global/c_session.h"

typedef struct LBRule {
	char vc_uuid[100];
	char protocol[100];
	char vc_ip[100];
	int port;
	char sched_name[100];
	int isHttpMeasure;
	int waf; //1 or 0
	struct LBRule *next;
}lbRule;

typedef struct IPRule
{
	char start_ip_range[100];
	char end_ip_range[100];
	char mask[100];
	struct IPRule *next;
}ipRule;

//10/13 add long type of virtual cluster id
typedef struct SLBInst {
	char vm_uuid[100];
	char vc_uuid[100];
	char vm_ip[100];
	char vc_ip[100];
	char phy_ip[100];
	int vm_status;
	long vc_id;
	long vdc_id;
	struct SLBInst *next;
}slbInst;

lbRule* getRulesByVCUUID(c_session *, char *);
slbInst* getInstByInstUUID(c_session *, char *);
slbInst* getSSHGWByVCUUID (c_session *, char *);
slbInst* getRDPGWByVCUUID (c_session *, char *);
lbRule* getRulesAll(c_session *);//all LBRule
slbInst* getInstAll(c_session *);//all SLBInst

// L2 integration add two API
long getVdcIdByVCUUID(c_session *, char *);
ipRule* getPerVdcRangeByVDCID(c_session *, long);

//todo: get cpu/memory loading data

#endif /* SLB_H_ */

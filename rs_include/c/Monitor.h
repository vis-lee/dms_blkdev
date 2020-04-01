/*
 * Monitor.h
 *
 *  Created on: Jun 29, 2010
 *      Author: root
 */

#include "../global/c_session.h"
#include "../global/api_version.h"

typedef struct InstPerf {
	char uuid[100];
	int cpuUsage;
	int memUsage;
	int networkSend;
	int networkRecv;
	int diskWrite;
	int diskRead;
	char createTime[100];
}instPerf;

typedef struct NodePerf {
	char ip[100];
	int cpuUsage;
	int memUsage;
	int networkSend;
	int networkRecv;
	int diskWrite;
	int diskRead;
	char createTime[100];
}nodePerf;

typedef struct HttpLatency
{
    unsigned int vc_id;
	char vm_ip[100];
	unsigned int msec;
	unsigned int rtimes;
	struct HttpLatency *next;
}httpLatency;

int createInstPerfByInstUUID(c_session *, instPerf *);
int createNodePerfByIP(c_session *, nodePerf *);
int sendHttpLatency2RS(c_session* session, httpLatency *);

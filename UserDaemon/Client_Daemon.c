/*
 * Client_Daemon.c
 *
 *  Created on: 2011/7/11
 *      Author: 980263
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <net/if.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>
#include <errno.h>
#include<fcntl.h>
#include "Client_Daemon.h"
#include "DMS_IOCTL.h"
#ifdef RS_SUPPORT
#include<global/c_session.h>
#include<c/DMS.h>
#endif

//#define TYP_INIT 0
//#define TYP_SMLE 1
//#define TYP_BIGE 2

//int devid[MAX_NUM_DMSDEV];

#ifdef RS_SUPPORT
//static c_session *rs_session = NULL;
static char* default_rs_name = "rs.ccma.itri";
static int default_rs_port = 9000;
static char* default_rs_account = "root";
static char* default_rs_pwd = "mysql";
char rs_name[32];
int rs_port;
char rs_account[32];
char rs_pwd[32];

void set_rs_setting_default() {
	memcpy(rs_name, default_rs_name, 32);
	rs_port = default_rs_port;
	memcpy(rs_account, default_rs_account, 32);
	memcpy(rs_pwd, default_rs_pwd, 32);
}

void set_rs_setting_from_config() {
	FILE *configfn;
	memset(rs_name, 0, 32);
	memset(rs_account, 0, 32);
	memset(rs_pwd, 0, 32);
	FILE *rs_configfn;
	syslog(LOG_ERR,"set_rs_setting_from_config file %s\n", RSConfigFile);
	if((rs_configfn = fopen(RSConfigFile, "r")) == NULL) {
		syslog(LOG_ERR,"Cannot open config file, use default setting\n");
		set_rs_setting_default();
		return;
	}

	char buff[256];
	char *delimiter = "=";
	char *tmp;
	char *pos1, *left = NULL, *right = NULL;
	int flag = 0;

	while(fgets(buff, 256, rs_configfn) != NULL) {
		buff[strlen(buff)-1] = '\0';

		pos1 = strstr(buff, delimiter);
		if(pos1 != NULL && pos1!=buff) {
			left = malloc(sizeof(char)*(strlen(buff) - strlen(pos1)+1));
			strncpy(left, buff, strlen(buff) - strlen(pos1));
			left[strlen(buff) - strlen(pos1)] = '\0';
			pos1 = pos1 + strlen(delimiter);
			right = malloc(sizeof(char)*(strlen(pos1)+1));
			strncpy(right, pos1, strlen(pos1));
			right[strlen(pos1)] = '\0';

			//printf("left=%s %d\n", left, strlen(left));
			//printf("right=%s %d\n", right, strlen(right));

			if(strcmp(left, "ip") == 0) {
				strncpy(rs_name, pos1, strlen(pos1));
				flag = flag | 1;
				printf("rs_name: %s\n", rs_name);
			} else if(strcmp(left, "port") == 0) {
				rs_port = atoi(right);
				flag = flag | 2;
				printf("rs_port: %d\n", rs_port);
			} else if(strcmp(left, "account") == 0) {
				memcpy(rs_account, pos1, strlen(pos1));
				flag = flag | 4;
				printf("rs_account: %s\n", rs_account);
			} else if(strcmp(left, "pwd") == 0) {
				memcpy(rs_pwd, pos1, strlen(pos1));
				flag = flag | 8;
				printf("rs_pwd: %s\n", rs_pwd);
			} else {
				set_rs_setting_default();
				break;
			}
		} else {
			set_rs_setting_default();
			break;
		}
	}
	syslog(LOG_ERR,"set_rs_setting_default finish flag = %d\n", flag);
	if(flag != 15)
	set_rs_setting_default();
	fclose(rs_configfn);
	if(left != NULL)
	free(left);
	if(right != NULL)
	free(right);
}

c_session* get_rs_connection() {
	int ret;
	c_session* rs_session=NULL;
	rs_session = (c_session*)malloc(sizeof(c_session));
	syslog(LOG_ERR,"DEBUG: rs setting: [name, port, account, pwd] = [%s, %d, %s, %s]\n", rs_name, rs_port, rs_account, rs_pwd);
	ret = initSession(rs_session, rs_name, rs_port, rs_account, rs_pwd);
	printf("after initSession: [isConnected, returnValue] = [%d, %d]\n", rs_session->isConnected, ret);
	if(ret < 0) {
		syslog(LOG_ERR,"cannot make a connection to rs\n");
		rs_session=NULL;
	} else
	syslog(LOG_ERR,"connect to rs ok\n");
	return rs_session;
}

int save_attach_info_to_rs(char* instID, char* volID, int isAttach) { //Lego: RSLib return 0 when success and -1 when fail
	int ret = 0;
	syslog(LOG_ERR,"save attach info to rs start\n");
	c_session* rsconn=get_rs_connection();
	int i;
	for(i=0;i<5;i++) {
		if(rsconn != NULL && checkConnection(rsconn) >= 0) {
			break;
		}
		rsconn=get_rs_connection();
		syslog(LOG_ERR,"Cannot connection rs, retry..... retry count=%d\n", i);
	}

	if(i==5) {
		syslog(LOG_ERR,"Cannot make connection with rs\n");
		return -1;
	}
	syslog(LOG_ERR,"ready to attach/detach %d volume %s to inst %s\n", isAttach, volID, instID);
	if(1 == isAttach) {
		syslog(LOG_ERR,"ready to call attachVolume\n");
		ret = attachVolume(rsconn, instID, volID);
		syslog(LOG_ERR,"attach %s to %s result: %d\n", instID, volID, ret);
	} else {
		syslog(LOG_ERR,"ready to call detachVolume\n");
		ret = detachVolume(rsconn, instID, volID);
		syslog(LOG_ERR,"detach %s to %s result: %d\n", instID, volID, ret);
	}

	if(ret == 0)
	syslog(LOG_ERR,"attach/detach %d volume %s to inst %s ok\n", isAttach, volID, instID);
	else
	syslog(LOG_ERR,"attach/detach %d volume %s to inst %s fail\n", isAttach, volID, instID);
	return ret;
}

void LegoTest(char* instID, char* volID, int isAttach) {
	printf("LegoTest start\n");
	save_attach_info_to_rs(instID, volID, isAttach);
}
#endif

void dumpAllAttach(struct VolAttachInfo * list) {
	struct VolAttachInfo * c = list;
	while (c->next != NULL) {
		c = c->next;
		printf("Attach: volume id[%d] instance id[%s] device path[%s]\n",
				c->VolumeID, c->InstanceID);
	}
}

char * getNamenodeAddress() {
	FILE *configfn;

	if ((configfn = fopen(NamenodeConfigFile, "r")) == NULL) {
		return NamenodeHostname;
	}

	char buff[256];
	char *startStr = "hdfs://";
	char *tmp;
	char *pos1, *pos2;
	while (fgets(buff, 256, configfn) != NULL) {
		pos1 = strstr(buff, startStr);
		if (pos1 != NULL) {
			pos1 = pos1 + strlen(startStr);
			pos2 = strstr(pos1, ":");
			if (pos2 != NULL) {
				tmp = (char*) malloc(sizeof(char) * (strlen(pos1)
						- strlen(pos2) + 1));
				memcpy(tmp, pos1, strlen(pos1) - strlen(pos2));
				tmp[strlen(pos1) - strlen(pos2)] = '\0';
				fclose(configfn);
				return tmp;
			}
		}
	}

	fclose(configfn);
	return NamenodeHostname;
}

int create_connection() {
	int ns = socket(AF_INET, SOCK_STREAM, 0); /* init socket descriptor */
	struct sockaddr_in sin;
	struct hostent *host;

	syslog(LOG_ERR, "Ready to establish connection to namodenode: %s\n",
			getNamenodeAddress());
	host = (struct hostent *) gethostbyname(getNamenodeAddress());
	while (host == NULL) {
		syslog(LOG_ERR, "retry gethostbyname %s\n", getNamenodeAddress());
		host = gethostbyname(getNamenodeAddress());
		sleep(1);
	}

	struct timeval timeout = { 3, 0 };
	int ret =
			setsockopt(ns, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
	if (ret < 0) {
		perror("setsockopt failed\n");
	} else {
		/*** PLACE DATA IN sockaddr_in struct ***/
		memcpy(&sin.sin_addr.s_addr, host->h_addr, host->h_length);
		sin.sin_family = AF_INET;
		sin.sin_port = htons(NamenodeWrapperPort);

		/*** CONNECT SOCKET TO THE SERVICE DESCRIBED BY sockaddr_in struct ***/
		if (connect(ns, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
			perror("connecting");
			ns = NULL;
			syslog(LOG_ERR, "Fail to establish connection\n");
			//exit(1);
		}
	}
	return ns;
}

void check_namenode_connection() {
	int error;
	int len;
	int ns = create_connection();
	syslog(LOG_ERR, "C Daemon is checking connection with namenode\n");
	while (getsockopt(ns, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
		ns = create_connection();
		syslog(LOG_ERR, "try to re-connect.....\n");
		sleep(1);
	}
	close(ns);
	syslog(LOG_ERR, "C Daemon connect to namenode ok!\n");
}

struct VolAttachInfo * QueryAttachInfo(struct VolAttachInfo * list, int volid) {
	struct VolAttachInfo *c = list;
	int i = 0;
	printf("Query volume ID %d\n", volid);
	while (c->next != NULL) {
		printf("Index %d\n", i++);
		c = c->next;
		if (c->VolumeID == volid) {
			printf("Query volume ID %d found %s!\n", c->VolumeID, c->InstanceID);
			return c;
		}
	}
	printf("Query volume ID %d not found!\n", volid);
	return NULL;
}

struct VolAttachInfo * AddAttachToList(struct VolAttachInfo * list, int volid,
		char* instID) {
	syslog(LOG_ERR, "AddAttachToList: %d\n", volid);
	syslog(LOG_ERR, "AddAttachToList: %s\n", instID);
	list_last->next = (struct VolAttachInfo *) malloc(
			sizeof(struct VolAttachInfo));
	list_last->next->VolumeID = volid;
	list_last->next->InstanceID = (char*) malloc(sizeof(char) * (strlen(instID)
			+ 1));
	memcpy(list_last->next->InstanceID, instID, strlen(instID));
	list_last->next->InstanceID[strlen(instID)] = '\0';
	list_last->next->next = NULL;
	//printf("AddAttachToList: %d\n", list_last->next->VolumeID);
	//printf("AddAttachToList: %d %s\n", strlen(list_last->next->InstanceID), list_last->next->InstanceID);
	//printf("AddAttachToList: %d %s\n", strlen(list_last->next->DevicePath), list_last->next->DevicePath);
	list_last = list_last->next;

	return list_last;
}

struct VolAttachInfo * RemoveAttachFromList(struct VolAttachInfo * list,
		int volid) {
	struct VolAttachInfo * c = list;
	struct VolAttachInfo * tmp = NULL;
	while (c->next != NULL) {
		if (c->next->VolumeID == volid) {
			if (c->next->next != NULL) {
				tmp = c->next;
				c->next = tmp->next;
				tmp->next = NULL;
				return tmp;
				//free(tmp);
			} else {
				//free(c->next);
				tmp = c->next;
				c->next = NULL;
				list_last = c;
				return tmp;
			}
			break;
		} else {
			c = c->next;
		}
	}

	return tmp;
}

char* ccma_buffer_read(int fd, int size) {
	char recv_buf[255];
	int ptr = 0, res = 0;
	int min = size < 255 ? size : 255;
	char* buf = (char*) malloc(sizeof(char) * size);

	while (ptr < size) {
		res = recv(fd, &recv_buf, min, MSG_WAITALL);

		if (res <= 0) {
			int retry = 5;
			while (retry > 0) {
				usleep(150);
				res = recv(fd, &recv_buf, min, MSG_WAITALL);
				if (res > 0) {
					syslog(LOG_ERR, "receive data retry... done\n");
					break;
				} else if (res < 0) {
					syslog(LOG_ERR, "receive data retry fail...errorno = %d\n",
							errno);
					retry--;
				} else
					syslog(LOG_ERR, "receive data retry fail...res=0\n");
			}

			if (retry <= 0) {
				syslog(LOG_ERR, "FATAL recv failed: %d = %d\n", res);
				free(buf);
				return NULL;
			}
		}
		memcpy(((char*) buf + ptr), recv_buf, res);
		ptr += res;
	}

	//return recv_buf; //commented by lego
	return buf;
}

void send_response(int sockfd, int retCode) {
	char *buf = (char*) (&retCode);

	write(sockfd, buf, 4);
}

long getCapacityFromNamenode(int volid) {
	int error;
	int len;
	int ns = create_connection();
	short opcode = htons(101);
	int volns = htonl(volid);
	long long magic_number = htonll(MAGIC_NUM);

	syslog(LOG_ERR, "Ready to getCapacity From Namenode with Volume id = %d\n",
			volid);
	while (getsockopt(ns, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
		ns = create_connection();
		syslog(LOG_ERR, "try to re-connect.....\n");
		sleep(1);
	}

	if (getsockopt(ns, SOL_SOCKET, SO_ERROR, &error, &len) >= 0) {
		write(ns, &(magic_number), sizeof(long));
		write(ns, &(opcode), sizeof(short));
		write(ns, &(volns), sizeof(int));
	}

	char * buf = ccma_buffer_read(ns, 8);

	unsigned long long capacity = 0;
	if (buf != NULL) {
		capacity = ntohll(*((unsigned long long*)buf));
		free(buf);
	}

	close(ns);
	return capacity;
}

int attach_volume(int vol_id, char* instid, struct VolAttachInfo *list,
		int updateToRS, int updateToLocalFile, int deviceNum) {
	long volCap;
	struct dms_volume_info vinfo;
	struct VolAttachInfo *item = QueryAttachInfo(list, vol_id);
	int ret, flag = 0;
	char *str_volid;
	char *dev_fn;

	//TODO check whether the volume has been attached or not.
	if (updateToLocalFile == 0) //when perform attach / detach all, don't query
		item = NULL;

	if (item != NULL) {
		syslog(LOG_ERR, "Volume id %d has already attach to instance id %s\n",
				item->VolumeID, item->InstanceID);
		return ATTACH_OK; // 0 represent attach ok
	} else {
		// get capacity from namenode
		syslog(LOG_ERR, "Ready to get capacity from Namenode\n");
		volCap = getCapacityFromNamenode(vol_id); //Lego: Should do a error handing once fail to get capacity
		if (volCap <= 0) {
			syslog(LOG_ERR,
					"Cannot get volume capacity or volume capacity is error\n");
			return ATTACH_FAIL; // 0 represent attach fail
		}
		syslog(LOG_ERR, "VolCap: %ld\n", volCap);
		//issue IOCTL to driver to attach and get device file name
		syslog(LOG_ERR, "Issue IOCTL to driver to attach volume\n");
		vinfo.capacity_in_bytes = volCap;
		vinfo.volid = vol_id;
		vinfo.deviceNum = deviceNum;
		ret = ioctl(io_fd, IOCTL_ATTACH_VOL, &vinfo); //should do error handling here

		if (ret != ATTACH_OK)
			return ret;
		else {
			//check device file
			dev_fn = (char*) malloc(sizeof(char) * 128);
			sprintf(dev_fn, "/dev/dms%ld\0", vol_id);
			syslog(LOG_ERR, "Ready check device file: %s\n", dev_fn);
			int retry = 0;
			while (retry < 30) {
				FILE* fp = fopen(dev_fn, "r");
				if (fp) {
					syslog(LOG_ERR,
							"Device files exist, close file and leave loop\n");
					flag = 1;
					//FIXME: fix read only issue?
					fclose(fp);
					break;
				} else {
					syslog(LOG_ERR, "Device files not exist, wait\n");
					flag = 0;
				}
				sleep(1);
			}

		}

		if (flag == 0) {
			syslog(LOG_ERR, "Attach fail due to device file %ld\n", vol_id);
			return ATTACH_FAIL;
		}
		//update RS
		if (updateToRS == 1) {
			syslog(LOG_ERR, "Ready to update RS\n");
#ifdef RS_SUPPORT
			str_volid = (char*)malloc(sizeof(char)*128);
			sprintf(str_volid, "%ld\0", vol_id);
			syslog(LOG_ERR,"Ready to save attach information to RS: %s %s\n", instid, str_volid);
			ret = save_attach_info_to_rs(instid, str_volid, 1);
			if(ret != 0) {
				syslog(LOG_ERR,"Update attach information to RS fail\n");
				return ATTACH_FAIL;
			}
#endif
		}

		if (updateToLocalFile == 1) {
			syslog(LOG_ERR, "Add attach information to local disk\n");
			item = AddAttachToList(list, vol_id, instid);
			SaveAttachInfoToFile(list);
			syslog(LOG_ERR, "Add attach information to local disk done!\n");
		}

		if (item != NULL) {
			syslog(LOG_ERR, "Item: %d %s\n", item->VolumeID, item->InstanceID);
			syslog(LOG_ERR, "Send response back\n");
			return ATTACH_OK;
		} else {
			syslog(LOG_ERR, "attach volume %ld fail!\n", vol_id);
			return ATTACH_FAIL;
		}
	}
	return ret;
}

void attach_all_volume(struct VolAttachInfo *list) {
	struct VolAttachInfo *c = list;

	while (c->next != NULL) {
		c = c->next;
		syslog(LOG_ERR, "attach: %d %s\n", c->VolumeID, c->InstanceID);
		syslog(LOG_ERR, "attach: device num: %d\n", c->VolumeID);
		attach_volume(c->VolumeID, c->InstanceID, list, 0, 0, c->VolumeID);
	}
}

int detach_volume(int vol_id, char* instid, struct VolAttachInfo *list,
		int updateToRS, int updateToLocalFile) {
	int ret = 1;
	char *str_volid;
	//issue IOCTL to driver to attach
	syslog(LOG_ERR, "Issue IOCTL to driver to dettach volume\n");
	ret = ioctl(io_fd, IOCTL_DETATCH_VOL, (unsigned long) vol_id);

	if (ret != DETACH_OK)
		return ret;
	//update RS
	if (updateToRS == 1) {
		syslog(LOG_ERR, "Ready to update RS\n");
#ifdef RS_SUPPORT
		str_volid = (char*)malloc(sizeof(char)*128);
		sprintf(str_volid, "%ld\0", vol_id);
		syslog(LOG_ERR,"Ready to save detach information to RS: %s %s\n", instid, str_volid);
		ret = save_attach_info_to_rs(instid, str_volid, 0);
		if(ret != 0) {
			syslog(LOG_ERR,"Update attach information to RS fail\n");
			return DETACH_FAIL;
		}
#endif
	}

	syslog(LOG_ERR, "Remove attach information from local disk\n");
	if (updateToLocalFile == 1) {
		if (RemoveAttachFromList(list, vol_id) == NULL)
			syslog(LOG_ERR, "Cannot revmoe attach information from linklist\n");
		if (SaveAttachInfoToFile(list) == 0)
			syslog(LOG_ERR, "Save attach information to file fail!\n");
	}

	//syslog(LOG_ERR,"Send response back");
	//send_response(clientfd, 0, NULL);
	return ret;
}

void detach_all_volume(struct VolAttachInfo *list) {
	struct VolAttachInfo *c = list;

	while (c->next != NULL) {
		//c = c->next;
		detach_volume(c->VolumeID, c->InstanceID, list, 0, 0);
	}
}

struct Volume_ACK_Policy {
	unsigned long VolumeID;
	int ACK_Policy;
};
int Change_Volume_ACK_Policy( volumeID, opcode) {
	//return ioctl(io_fd, IOCTL_FLUSH_VOL,(unsigned long)vol_id);
	return -1;
}

/*
 * process_thread : handle commands
 * 1. attach volume : packsize Opcode volumeID instanceID (Opcode = 1 and 3)
 * 2. detach volume : packsize  Opcode volumeID (Opcode = 2 and 4)
 * 3. reset overwritten flag : packsize Opcode (Opcode = 11)
 * 4. reset cache : packsize Opcode (Opcode = 12) volumeID
 * 5. attach all volume : packsize Opcode (Opcode = 5)
 * 6. detach all volume : packsize Opcode (Opcode = 6)
 * 7. attach volume id on device id : packsize  Opcode volumeID InstanceID deviceID (Opcode = 7, long, int)
 * 8. change volume ack policy to DISK_ACK : packsize  Opcode volumeID  (Opcode = 8, long)
 * 9. change volume ack policy to MEMORY_ACK : packsize  Opcode volumeID  (Opcode = 9, long)
 *
 * packsize : 2 bytes
 * opcode : 2 bytes, volume id : 8 bytes, instanceID : 22 bytes
 * response code 0 : OK, 1 : fail, 2 : OK but not update to RS
 */
void process_thread(int clientfd) {
	char* buf = ccma_buffer_read(clientfd, 2);
	short int PktLen, opcode;
	int retCode;
	int requestID = -1;
	PktLen = ntohs(*((short int*) buf));
	int i;

	buf = ccma_buffer_read(clientfd, 2);
	opcode = ntohs(*((short int*) buf));

	syslog(LOG_ERR, "Receive command with opcode %d packet length %d\n",
			opcode, PktLen);
	if (opcode == 11) {
		//send ioctl command to driver to reset overwritten flag
		buf = ccma_buffer_read(clientfd, 4);
		requestID = ntohl(*((int*) buf));
		buf = ccma_buffer_read(clientfd, 8);
		unsigned long long volID = ntohll(*((unsigned long long*)buf));
		syslog(
				LOG_ERR,
				"Ready to send reset overwritten flag command to driver with request id = %d and volume id = %ld\n",
				requestID, volID);
		short ackCode = ioctl(io_fd, IOCTL_RESETOVERWRITTEN,
				(unsigned long) volID);
		syslog(LOG_ERR, "Ack code from driver %d\n", ackCode);
		short retPktlen = 6;
		buf = (char*) malloc(sizeof(char) * 8);
		memcpy(buf, (char*) (&retPktlen), 2);
		memcpy(buf + 2, (char*) (&ackCode), 2);
		memcpy(buf + 4, (char*) (&requestID), 4);
		write(clientfd, buf, 8);
	} else if (opcode == 12) {
		buf = ccma_buffer_read(clientfd, 4);
		int subOpcode = ntohl(*((int*) buf));
		buf = ccma_buffer_read(clientfd, 4);
		requestID = ntohl(*((int*) buf));
		struct ResetCacheInfo * resetInfo = (struct ResetCacheInfo*) malloc(
				sizeof(struct ResetCacheInfo));
		syslog(
				LOG_ERR,
				"Receive clear meta cache command with request id %d and subopcode %d\n",
				requestID, subOpcode);
		resetInfo->SubOpCode = subOpcode;
		if (subOpcode == SUBOP_INVALIDATECACHE_BYHBIDS) {
			syslog(LOG_ERR, "invalidate cache by HBIDs\n");
			buf = ccma_buffer_read(clientfd, 8);
			unsigned long long volID = ntohll(*((unsigned long long*)buf));
			syslog(LOG_ERR, "volume id = %ld\n", volID);
			buf = ccma_buffer_read(clientfd, 4);
			unsigned int NumOfHBIDs = ntohl(*((unsigned int*) buf));
			buf = ccma_buffer_read(clientfd, NumOfHBIDs * 8);

			resetInfo->u.HBIDs.VolumeID = volID;
			resetInfo->u.HBIDs.NumOfLBID = NumOfHBIDs;
			resetInfo->u.HBIDs.LBIDs = (unsigned long long*) buf;
			syslog(LOG_ERR, "num of HBIDs %d and HBIDs as following: \n",
					NumOfHBIDs);
			for (i = 0; i < resetInfo->u.HBIDs.NumOfLBID; i++)
				printf("HBID: %ld\n", resetInfo->u.HBIDs.LBIDs[i]);
			short ackCode = ioctl(io_fd, IOCTL_INVALIDCACHE,
					(unsigned long) resetInfo);
			short retPktlen = 6;
			buf = (char*) malloc(sizeof(char) * 8);
			memcpy(buf, (char*) (&retPktlen), 2);
			memcpy(buf + 2, (char*) (&ackCode), 2);
			memcpy(buf + 4, (char*) (&requestID), 4);
			write(clientfd, buf, 8);
		} else if (subOpcode == SUBOP_INVALIDATECACHE_BYVOLID) {
			syslog(LOG_ERR, "invalidate cache by volume id\n");
			buf = ccma_buffer_read(clientfd, 8);
			unsigned long long volID = ntohll(*((unsigned long long*)buf));
			syslog(LOG_ERR, "volume id = %ld\n", volID);
			resetInfo->u.VolID.VolumeID = volID;
			short ackCode = ioctl(io_fd, IOCTL_INVALIDCACHE,
					(unsigned long) resetInfo);
			short retPktlen = 6;
			buf = (char*) malloc(sizeof(char) * 8);
			memcpy(buf, (char*) (&retPktlen), 2);
			memcpy(buf + 2, (char*) (&ackCode), 2);
			memcpy(buf + 4, (char*) (&requestID), 4);
			write(clientfd, buf, 8);
		} else if (subOpcode == SUBOP_INVALIDATECACHE_BYDNNAME) {
			syslog(LOG_ERR, "invalidate cache by datanode\n");
			buf = ccma_buffer_read(clientfd, 4);
			unsigned int dnPort = ntohl(*((int*) buf));
			syslog(LOG_ERR, "datanode port: %d\n", dnPort);

			buf = ccma_buffer_read(clientfd, 4);
			unsigned int dnNameLen = ntohl(*((int*) (buf)));
			syslog(LOG_ERR, "datanode name len: %d\n", dnNameLen);

			buf = ccma_buffer_read(clientfd, dnNameLen);
			syslog(LOG_ERR, "len : %d\n", strlen(buf));
			syslog(LOG_ERR, "datanode name %s\n", buf);

			short retPktlen = 6;
			resetInfo->u.DNName.Port = dnPort;
			resetInfo->u.DNName.NameLen = dnNameLen;
			resetInfo->u.DNName.Name = (char*) malloc(sizeof(char) * dnNameLen);
			memcpy(resetInfo->u.DNName.Name, buf, dnNameLen);
			//printf("port = %d  len = %d  name = %s\n", dnPort, dnNameLen, resetInfo->u.DNName.Name);
			short ackCode = ioctl(io_fd, IOCTL_INVALIDCACHE,
					(unsigned long) resetInfo);
			buf = (char*) malloc(sizeof(char) * 8);
			memcpy(buf, (char*) (&retPktlen), 2);
			memcpy(buf + 2, (char*) (&ackCode), 2);
			memcpy(buf + 4, (char*) (&requestID), 4);
			write(clientfd, buf, 8);
		} else {
		}

		free(resetInfo);
	} else if (opcode == 1 || opcode == 3) { //attach volume
		syslog(LOG_ERR, "Ready to attach volume\n");
		buf = ccma_buffer_read(clientfd, 8);
		unsigned long long volID = *((unsigned long long*) buf);

		if (PktLen > 10) {
			buf = ccma_buffer_read(clientfd, PktLen - 2 - 8);
			char* tmp = (char*) malloc(sizeof(char) * (PktLen - 10));
			memcpy(tmp, buf, (PktLen - 10));
			char *instID;
			int len = strlen(tmp);

			instID = (char*) malloc(sizeof(char) * (len + 1));
			memcpy(instID, tmp, len);
			instID[len] = '\0';
			syslog(LOG_ERR,
					"Commands: attach volume [id=%ld] to instance [id=%s]\n",
					volID, instID);

			if (opcode == 1) {
				retCode = attach_volume(volID, instID, list_VolAttachInfo, 1,
						1, -1);
			} else {
				retCode = attach_volume(volID, instID, list_VolAttachInfo, 0,
						1, -1);
			}
			syslog(LOG_ERR, "Process thread: /dev/dms%d\n", volID);
			if (retCode == 0)
				send_response(clientfd, 0);
			else
				send_response(clientfd, 1);

			free(tmp);
		} else {
			syslog(LOG_ERR, "attach commands format error!\n"); //send error code to sender
			send_response(clientfd, 1);
		}
	} else if (opcode == 2 || opcode == 4) { //detach volume
		syslog(LOG_ERR, "Ready to detach volume\n");
		buf = ccma_buffer_read(clientfd, 8);
		unsigned long long volID = *((unsigned long long*) buf);
		int ret = 0;
		if (PktLen > 10) {
			buf = ccma_buffer_read(clientfd, PktLen - 2 - 8);
			char* tmp = (char*) malloc(sizeof(char) * (PktLen - 10));
			memcpy(tmp, buf, (PktLen - 10));

			char *instID;
			int len = strlen(tmp);
			instID = (char*) malloc(sizeof(char) * (len + 1));
			memcpy(instID, tmp, len);
			instID[len] = '\0';
			syslog(
					LOG_ERR,
					"Commands: detach volume [id=%ld] to instance [id=%s] without device file\n",
					volID, instID);

			if (opcode == 2)
				ret = detach_volume(volID, instID, list_VolAttachInfo, 1, 1);
			else
				ret = detach_volume(volID, instID, list_VolAttachInfo, 0, 1);
			if (ret == 0)
				send_response(clientfd, 0);
			else
				send_response(clientfd, 1);
			free(tmp);
		} else {
			syslog(LOG_ERR, "attach commands format error!\n"); //send error code to sender
			send_response(clientfd, 1);
		}
	} else if (opcode == 5 || opcode == 6) { //attach/detach all volume
		syslog(LOG_ERR, "Ready to attach/detach all volume");
		if (opcode == 5)
			attach_all_volume(list_VolAttachInfo);
		else
			detach_all_volume(list_VolAttachInfo);
	} else if (opcode == 7) {
		syslog(LOG_ERR, "Ready to attach volume id on device id");
	} else if (opcode == DMS_CMD_ACK_POLICY_DISK_RESPONSE || opcode
			== DMS_CMD_ACK_POLICY_MEMORY_RESPONSE) {
		//TODO Change_Volume_ACK_Policy(volumeID, opcode);
		send_response(clientfd, -ENOTTY);
	} else {
		syslog(LOG_ERR, "unknown commands, skip process!\n");
	}
}

void create_socket_server() {
	struct sockaddr_in dest;
	int on = 0;

	/* create socket */
	sock_listen = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));

	/* initialize structure dest */
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(SERVER_PORT);

	/* this line is different from client */
	dest.sin_addr.s_addr = INADDR_ANY;

	/* assign a port number to socket */
	bind(sock_listen, (struct sockaddr*) &dest, sizeof(dest));

	/* make it listen to socket with max 20 connections */
	listen(sock_listen, 30);

	while (1) {
		int clientfd;
		struct sockaddr_in client_addr;
		int addrlen = sizeof(client_addr);

		/* Wait and accept connection */
		clientfd = accept(sock_listen, (struct sockaddr*) &client_addr,
				&addrlen);
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_t id;
		int i, ret;
		ret = pthread_create(&id, &attr, (void*) process_thread, clientfd);
		if (ret != 0) {
			printf("Socket server create pthread error!\n");
			exit(1);
		}

		pthread_attr_destroy(&attr);
	}
}

void auto_remove_driver() {
	FILE *fp = popen("lsmod|grep dms_vdd", "r");
	char buf[200];

	if (fgets(buf, 200, fp) != EOF) {
		if (buf[0] == 0) {
			printf("driver already unloaded..\n");
		} else {
			printf("unload driver \n");
			system("rmmod dms_vdd"); //Lego: should issue a ioctl to notify driver stop IO before remove module
			printf("driver removed\n");
		}
	}
	pclose(fp);
}

void create_socket_server_in_background() {
	/*pthread_t id;

	 int i, ret;
	 ret = pthread_create(&id, NULL, (void *)create_socket_server, NULL);
	 if(ret != 0) {
	 printf("Create pthread error!\n");
	 exit(1);
	 }*/
	create_socket_server();
}

void check_driver() {
	char buf[200];
	struct hostent *nn_host;
	struct sockaddr_in nn_Addr;
	char nn_ip_addr[16];
	char cmd[128];
	unsigned char *ucp;

	printf("check_driver\n");
	FILE *fp = popen("lsmod|grep ccma", "r");

	nn_host = (struct hostent *) gethostbyname(getNamenodeAddress());
	if (nn_host == NULL) {
		syslog(LOG_ERR, "cannot get ip from hostname : %s\n",
				getNamenodeAddress());
		exit(1);
	}

	memcpy(&(nn_Addr.sin_addr.s_addr), nn_host->h_addr, nn_host->h_length);
	ucp = (unsigned char *) &nn_Addr.sin_addr;
	sprintf(nn_ip_addr, "%d.%d.%d.%d", ucp[0] & 0xff, ucp[1] & 0xff, ucp[2]
			& 0xff, ucp[3] & 0xff);
	printf("The namenode ip addr is %s\n", nn_ip_addr);
	memset(buf, 0, 200);
	if (fgets(buf, 200, fp) != EOF) {
		if (buf[0] == 0) {
			sprintf(cmd, "insmod ./%s.ko namenode_ip=\"%s\"", DMS_DEV_NAME, nn_ip_addr);

			printf("driver loading with command: %s....\n", cmd);
			system(cmd);
			printf("driver loaded..\n");
		} else {
			printf("buf 0=%d %d\n", buf[0], buf[1]);
			printf("the driver already loaded\n");
		}
	}
	pclose(fp);
}

void check_driver_in_background() {
	pthread_t id;
	int i, ret;

	ret = pthread_create(&id, NULL, (void *) check_driver, NULL);

	if (ret != 0) {
		printf("Create pthread error!\n");
		exit(1);
	}
}


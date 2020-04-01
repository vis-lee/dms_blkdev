/*
 * Client_Daemon.h
 *
 *  Created on: 2011/7/11
 *      Author: 980263
 */

#ifndef CLIENT_DAEMON_H_
#define CLIENT_DAEMON_H_


#include <linux/ioctl.h>


#define DMSD_NAME			"dmsd"

#define SERVER_PORT	9898
int sock_listen;


#define DMS_CMD_ATTACH_VOLUME 					"1"
#define DMS_CMD_DETACH_VOLUME					"2"
#define DMS_CMD_ATTACH_VOLUME_FOR_MIGRATION		"3"
#define DMS_CMD_DETACH_VOLUME_FOR_MIGRATION		"4"
#define DMS_CMD_ATTACH_ALL_VOLUME 				"5"
#define DMS_CMD_DETACH_ALL_VOLUME				"6"
#define DMS_CMD_ACK_POLICY_DISK_RESPONSE		"8"
#define DMS_CMD_ACK_POLICY_MEMORY_RESPONSE		"9"


#define VOLATTACH_FN  "VolVMAttach.txt"
#define NamenodeWrapperPort 1234
#define NamenodeHostname  "dms.ccma.itri"
#define NamenodeConfigFile  "conf/core-site.xml"
#ifdef RS_SUPPORT
#define RSConfigFile "/usr/cloudos/dms/conf/rs.conf"
#endif
#define MAGIC_NUM 0xa5a5a5a5a5a5a5a5ll

#define htonll(x) \
((((x) & 0xff00000000000000LL) >> 56) | \
(((x) & 0x00ff000000000000LL) >> 40) | \
(((x) & 0x0000ff0000000000LL) >> 24) | \
(((x) & 0x000000ff00000000LL) >> 8) | \
(((x) & 0x00000000ff000000LL) << 8) | \
(((x) & 0x0000000000ff0000LL) << 24) | \
(((x) & 0x000000000000ff00LL) << 40) | \
(((x) & 0x00000000000000ffLL) << 56))

#define ntohll(x)	htonll(x)

int io_fd;


struct VolAttachInfo {
  unsigned long VolumeID;
  char* InstanceID;
  struct VolAttachInfo *next;
};
struct VolAttachInfo *list_VolAttachInfo;
struct VolAttachInfo *list_last;

int SaveAttachInfoToFile(struct VolAttachInfo *list);
//Load attach information from file
int LoadAttchInfoFromFile(struct VolAttachInfo *list);
int initialDeviceFile();
void attach_all_volume(struct VolAttachInfo *list);
void detach_all_volume(struct VolAttachInfo *list);
void dumpAllAttach(struct VolAttachInfo * list);
#ifdef RS_SUPPORT
void set_rs_setting_from_config();
int init_rs_connection();
void LegoTest(char* instID, char* volID, int attach);
#endif

#endif /* CLIENT_DAEMON_H_ */

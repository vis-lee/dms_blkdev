/*
 * Client_Daemon_Skeleton.c
 *
 *  Created on: 2011/7/11
 *
 * Author: Lego Lin
 * Date  : 2010/08/19
 * Description: CDaemon run at computer node. Respobsility as following:
 * 1. Load block driver automatically. Remove block driver when receive control-c signal
 * 2. Run as a socket server to accept commands from Namenode and VMM
      2.1 Command: reset overwritten flag from Namenode
      2.2 Command: attach volume from VMM
      2.3 Command: detach volume from VMM
   3. update attach / detach information to RS and load information at startup.
   4. Pass attach / detach / reset overwritten flag commands to driver
   5. Automatically attach / detach at startup / shutdown
   6. maintain the mapping of VolumeID InstanceID DeviceFile:
      7.1 If user attach the same instance twice, we can return the file immedidatly
      7.2 Automatically attach when startup
      7.3 Automatically detach when shutdown

   Design Note
   1. One major thread running:
      1.1 Main thread: accept commands
   2. work thread
      2.1 create at runtime
      2.2 update RS
      2.3 handle command
 */

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<syslog.h>
#include<fcntl.h>
#include<errno.h>
#include <sys/mman.h>
#include "Client_Daemon.h"
#include "DMS_IOCTL.h"
char auto_load_driver=0;
int MAX_DATANODE_REQ_QUEUE;
//int netlink_fd;

void init_netlink_server(){
//	netlink_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_TEST);
}

int LoadAttachInfoFromFile(struct VolAttachInfo *list) {
  FILE *attachFN;

  printf("Ready to LoadAttachInfoFromFile\n");
  if((attachFN = fopen(VOLATTACH_FN, "r")) == NULL) {
    printf("File not exist, create one\n");
    if(fopen(VOLATTACH_FN, "w+") == NULL) {
      syslog(LOG_ERR, "Cannot read file from %s\n", VOLATTACH_FN);
      return 0;
    }
    return 1;
  }

  int volid = -1;
  char instID[64], devName[128];
  char buff[256];
  struct VolAttachInfo *c = list;
  while(fgets(buff, 256, attachFN) != NULL) {
    printf("ReadLine: %s", buff);
    sscanf(buff, "%d %s\n", &volid, instID);
    if(volid > -1) {
      struct VolAttachInfo * curr = (struct VolAttachInfo *)malloc(sizeof(struct VolAttachInfo));
      curr->VolumeID = volid;
      curr->InstanceID = (char*)malloc(sizeof(char)*(strlen(instID)+1));
      memcpy(curr->InstanceID, instID, strlen(instID));
      curr->InstanceID[strlen(instID)] = '\0';
      curr->next = NULL;
      c->next = curr;
      c = c->next;
      list_last = c;
      printf("Add Record: %d %s\n", curr->VolumeID, curr->InstanceID);
    }
  }

  fclose(attachFN);
  return 1;
}

int SaveAttachInfoToFile(struct VolAttachInfo *list) {
  FILE *attachFN;

  syslog(LOG_ERR,"Ready to SaveAttachInfoToFile\n");
  if((attachFN = fopen(VOLATTACH_FN, "w+")) == NULL) {
    syslog(LOG_ERR, "Cannot read file from %s\n", VOLATTACH_FN);
    return 0;
  }

  struct VolAttachInfo *c = list;
  while(c->next != NULL) {
    c = c->next;
    fprintf(attachFN, "%d %s\n", c->VolumeID, c->InstanceID);
    syslog(LOG_ERR,"Add Record: %d  %s\n", c->VolumeID, c->InstanceID);
  }

  fclose(attachFN);

  return 1;
}

//void my_mutex_destroy() {
//  pthread_mutex_destroy(j_mutex);
//}

void connection_broken(){
  syslog(LOG_ERR,"connection  was broken\n");
//	exit(0);
}

void sighandler(int sig)

{
  printf("control-c pressed\n");

  //detach_all_volume(list_VolAttachInfo);

	ioctl(io_fd, IOCTL_RESET_DRIVER,33);


	//if(global_file!=NULL)
  //  fclose(global_file);
	//my_mutex_destroy();

  close(io_fd);
#ifdef MY_MMAP
  exit_mmap();
#endif
  auto_remove_driver();
	exit(1);
}

int initialize() {

	char devname[32] = {0};
	char syscmd[128] = {0};

	syslog(LOG_ERR,"Initial DMS clientnode setting\n");
	signal(SIGINT, &sighandler);
	//signal(SIGTSTP, &tsphandler);
	signal(SIGPIPE, &connection_broken);

	if(auto_load_driver) {
		syslog(LOG_ERR,"Initialize: ready to load driver\n");
		//check_driver_in_background();
		check_driver();
		syslog(LOG_ERR,"Initialize: driver load OK\n");
	}

	sprintf(&devname, "/dev/%s", DMS_CONTROLLER_NAME);

	FILE *fp=fopen(&devname,"r");
	if(!fp){
		sprintf(&syscmd, "mknod %s c %d 0", &devname, DMS_CONTROLLER_MAJOR);
		system(&syscmd);
		syslog(LOG_INFO,"%s: %s\n", &DMS_CONTROLLER_NAME, &syscmd);
	}else{
		fclose(fp);
	}

	syslog(LOG_ERR,"Initialize: %s checked!\n", &devname);
	io_fd= open(&devname, O_RDWR);
	if (io_fd == -1) {
		syslog(LOG_ERR,"create character device %s fail\n", &devname);
		perror("Error opening file for writing");
		exit(0);
	}
	syslog(LOG_INFO,"Initialize: get %s file descriptor\n", &devname);
	list_VolAttachInfo = (struct VolAttachInfo *)malloc(sizeof(struct VolAttachInfo));
	list_VolAttachInfo->VolumeID = -1;
	list_VolAttachInfo->next = NULL;
	list_last = list_VolAttachInfo;
	syslog(LOG_ERR,"Initialize: initial list\n");

	init_procfs();

//  LoadAttachInfoFromFile(list_VolAttachInfo);
	syslog(LOG_ERR,"Initialize: LoadAttachInforFromFile Done!\n");
  //attach_all_volume(list_VolAttachInfo);
	syslog(LOG_ERR,"Initialize: attach_all_volume Done!\n");
	return 0;
}


char* locate_configuration(){
	const char* dir_usr="/usr/cloudos/dms/conf/client.conf";
	int fd=open(dir_usr,O_EXCL);
	if(fd>=0){
		close(fd);
		return dir_usr;
	}
	printf("fid=%d\n",fd);
	close(fd);
	return NULL;
}

inline read_fields(char * str,char* cmp,int* result){
	char* ptr=index(str,'=');
	if( strncmp(str,cmp,ptr-str)==0){
		sscanf(ptr,"=%d",result);
	}
	printf("read_fields=%d\n",*result);
}

void write_procfs(char *fname,int val){
	char buf[100],valbuf[20];
	sprintf(buf,"/proc/ccma/%s",fname);
	FILE *fp=fopen(buf,"w");
	int flag = 0;
	int retry=0;
	while(retry < 30) {
		if(fp != NULL) {
			sprintf(valbuf,"%d",val);
			fprintf(fp,"%s",valbuf);
			flag = 1;
			break;
		} else {
			sleep(1);
		}
		retry++;
	}
	if(flag)
		fclose(fp);
}

void init_procfs(){
	write_procfs("max_datanode_req_queue",MAX_DATANODE_REQ_QUEUE);
}

void init_conf_values(){
	MAX_DATANODE_REQ_QUEUE=1200;
}


void load_configuration(){
	char *fname=locate_configuration();
	char buf[1024];
	if(fname!=NULL){
		printf("fname=%s\n",fname);
		FILE *fp=fopen(fname,"r");
		fgets(buf,1024,fp);
		printf("%s",buf);
		read_fields(buf,"MAX_DATANODE_REQ_QUEUE",&MAX_DATANODE_REQ_QUEUE);

		fclose(fp);
	}
}


int main(int argc, char* argv[]) {
  char hostname[64];
  struct hostent *host;

	syslog(LOG_ERR,"User level daemon start up\n");
//	system("sudo sysctl -w vm.vfs_cache_pressure=4096");
	init_conf_values();
	auto_load_driver=1;
	load_configuration();
	check_namenode_connection();

#ifdef RS_SUPPORT
  set_rs_setting_from_config();
  //init_rs_connection();
#endif
  syslog(LOG_ERR,"Finish setting rs configuration from config file\n");
	if(initialize() < 0) {
		syslog(LOG_ERR,"User level initialize fail\n");
		return -1;
	}

  memset(hostname, 0, 64);
  hostname[0] = '\0';
#ifdef RS_SUPPORT
  //LegoTest("i-FECE86D3", "206204952", 1);
#endif
	create_socket_server_in_background();  //create socket server to run at port 9898 for accepting commands from namenode and VMM

	return 0;
}

/*
 * DMS_Mem_Pool.h
 *
 *  Created on: 2011/7/14
 *      Author: 980263
 */

#ifndef DMS_MEM_POOL_H_
#define DMS_MEM_POOL_H_

#include <linux/types.h>


/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/


struct DMS_Mempool{

		kmem_cache_t *cachep;

		mempool_t *poolp;

};

struct DMP_Manager {

		char *dmp_name;

		size_t obj_size;

		struct DMS_Mempool *dmpp;

};

/********************************************************************************/
/*																				*/
/*								FUNCTIONS 										*/
/*																				*/
/********************************************************************************/
/*
 * DMS defined malloc, the original type of size_t is unsigned long.
 */
void * DMS_Malloc(size_t size);
void * DMS_Malloc_NOWAIT(size_t size);
void * _DMS_Malloc_Generic(size_t size, gfp_t flags);

/* dms volume memory allocator */
void * DMS_Volume_Malloc(size_t size);
void DMS_Volume_Free(void *ptr);

/*
 * DMS Memory Manager APIs
 */
int Init_DMP_Manager(void);
void Release_DMP_Manager(void);


/********************************************************************************/
/*																				*/
/*							COMMON ALLOC/FREE FUNCS 							*/
/*																				*/
/********************************************************************************/
struct DList_Node * Malloc_DLN ( gfp_t flags );
void Free_DLN(struct DList_Node *dln);

struct DList * Malloc_DList ( gfp_t flags );
void Free_DList(struct DList *dlist);

struct DMS_IO * Malloc_DIO ( gfp_t flags );
void Free_DIO(struct DMS_IO *dio);

struct LogicalBlock_MetaData * Malloc_LBMD ( gfp_t flags );
void Free_LBMD(struct LogicalBlock_MetaData *lbmd);

struct LogicalBlock_List * Malloc_LB_List ( gfp_t flags );
void Free_LB_List(struct LogicalBlock_List *ior);

struct IO_Request * Malloc_IO_Request ( gfp_t flags );
void Free_IO_Request(struct IO_Request *ior);

struct DMS_Node_Container * Malloc_DMS_Node_Container ( gfp_t flags );
void Free_DMS_Node_Container(struct DMS_Node_Container *dnc);

struct Datanode_Location * Malloc_Datanode_Location ( gfp_t flags );
void Free_Datanode_Location(struct Datanode_Location *dn_loc);

struct Located_Request * Malloc_Located_Request ( gfp_t flags );
void Free_Located_Request(struct Located_Request *lr);

struct DMS_Metadata * Malloc_DMS_Metadata ( gfp_t flags );
void Free_DMS_Metadata(struct DMS_Metadata *dmeta);

struct DMS_Protocol_Header * Malloc_NN_Protocol_Header ( gfp_t flags );
void Free_NN_Protocol_Header(struct DMS_Protocol_Header *header);

struct DMS_Datanode_Tag * Malloc_DMS_Datanode_Tag ( gfp_t flags );
void Free_DMS_Datanode_Tag(struct DMS_Datanode_Tag *dntag);

struct DMS_Payload_Tag * Malloc_DMS_Payload_Tag ( gfp_t flags );
void Free_DMS_Payload_Tag(struct DMS_Payload_Tag *dpayload);

#endif /* DMS_MEM_POOL_H_ */

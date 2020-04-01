/*
 * DMS_Error.c
 *
 *  Created on: Feb 13, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */


#include "DMS_Error.h"


struct DMS_Error_Log_String_Entry err_log_Map[] = {
		{EDEV_SYS_PENDING, "DMS_VDD are pending. "},
		{EDEV_NOP, 			"DMS_VDD not supported operation. "},
		{EDEV_NOMEM, 		"No Memory. "},
		{EVMGR_OUT_OF_RANGE, "volume minor index out of range! "},
		{EVMGR_CKS, 		"create kernel structures fail! "},
		{EDN_EDNS, 			"ALL DNs are failed! "},
		{0, ""}
};




/*
 * decode common network errno values into more useful strings.
 * strerror would be nice right about now.
 */
const char *__errorntostr(int errno) {

		switch (errno) {

			case 1:
				return "code 1";
			case DMS_OK:
				return "code 0, OK~";
			case -EIO:
				return "I/O error";
			case -EINTR:
				return "Interrupted system call";
			case -ENXIO:
				return "No such device or address";
			case -EFAULT:
				return "Bad address";
			case -EBUSY:
				return "Device or resource busy";
			case -EINVAL:
				return "Invalid argument";
			case -EPIPE:
				return "Broken pipe";
			case -ENONET:
				return "Machine is not on the network";
			case -ECOMM:
				return "Communication error on send";
			case -EPROTO:
				return "Protocol error";
			case -ENOTUNIQ:
				return "Name not unique on network";
			case -ENOTSOCK:
				return "Socket operation on non-socket";
			case -ENETDOWN:
				return "Network is down";
			case -ENETUNREACH:
				return "Network is unreachable";
			case -ENETRESET:
				return "Network dropped connection because of reset";
			case -ECONNABORTED:
				return "Software caused connection abort";
			case -ECONNRESET:
				return "Connection reset by peer";
			case -EISCONN:
				return "Transport endpoint is already connected";
			case -ESHUTDOWN:
				return "Cannot send after shutdown";
			case -ETIMEDOUT:
				return "Connection timed out";
			case -ECONNREFUSED:
				return "Connection refused";
			case -EHOSTDOWN:
				return "Host is down";
			case -EHOSTUNREACH:
				return "No route to host";
			case -EALREADY:
				return "Operation already in progress";
			case -EDNC_FORMAT:
				return "The ip format is invalid";
			case -EDNC_CONNECT:
				return "Connection Broken!";
			case -EDNC_RC_OVTIME:
				return "The rebuild counter over ONE DAY";
			case -EDNC_SNDTIMEO:
				return	"Socket send time out!";
			case -EDNC_RECVTIMEO:
				return "Socket receive time out!";
			default:
				return "Unknow error code";
		}

}

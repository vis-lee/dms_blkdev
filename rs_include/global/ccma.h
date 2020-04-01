/*
 * ccma.h
 *
 *  Created on: 2010/5/4
 *      Author: root
 */

#ifndef CCMA_H_
#define CCMA_H_

//comment out below flag if no google test
//#define __GOOGLE_TEST__

#include <string>
#include <stdlib.h>
#include <functional>
#include <list>
#include <vector>
#include <map>

using namespace std;

enum CCMAObjectType{NODE, USER, DATA_CENTER, CLUSTER, INSTANCE, VOLUME, LB_POLICY, LB_POLICY_RULE, NODE_PERFORMANCE, INST_PERFORMANCE, BACKUP, NETWORK, BACKUP_POLICY, IP_POLICY_RULE, VM_PM};
enum CCMAObjectDAO{DAO_INSERT,DAO_UPDATE, DAO_SUB_UPDATE, DAO_DELETE};
//enum NODEATTR{NODEATTR_UNKNOWN, NODEATTR_COMMENT, NODEATTR_POSITION, NODEATTR_TYPE, NODEATTR_STATUS, NODEATTR_HWINFO};
//enum NODEHWINFO{NODEHWINFO_UNKNOWN, NODEHWINFO_BMC, NODEHWINFO_CPU, NODEHWINFO_DISK, NODEHWINFO_NIC};
//enum NODESTATUS{NODESTATUS_NEW, NODESTATUS_INSTALL_SUCCESS, NODESTATUS_INSTALL_FAILED, NODESTATUS_ON, NODESTATUS_OFF, NODESTATUS_PANIC, NODESTATUS_FORCE_OFF};
//enum NODETYPE{NODETYPE_SERVICENODE, NODETYPE_COMPUTENODE, NODETYPE_STORAGENODE};

#define CCMAErrorCode_childlist_must_be_empty (-100)
#define CCMAErrorCode_criteria_addCriterion1 (-200)
#define CCMAErrorCode_criteria_addCriterion2 (-201)
#define CCMAErrorCode_criteria_addCriterion3 (-202)
#define CCMAErrorCode_criteria_addCriterion4 (-203)
#define CCMAErrorCode_dao_bad_cast (-204)
#define CCMAErrorCode_dao_executeQuery (-205)
#define CCMAErrorCode_dao_connection_null (-206)
#define CCMAErrorCode_connection_fail (-207)
#define CCMAErrorCode_memory_allocated_fail (-208)
#define CCMAErrorCode_null_pointer_fail (-209)
#define CCMAErrorCode_mutex_init_fail (-210)
#define CCMAErrorCode_schema_version_check_fail (-211)
#define CCMAErrorCode_api_version_check_fail (-212)
#define RS_MEMORY_FAIL "RS_MEMORY_ALLOCATED_FAIL"
#define CCMAErrorCode_duplicate_volume (-213)

#define 	PRM		0x0001
#define		SNM		0x0002
#define		VDCM	0x0004
#define		RPM		0x0008
#define		RS		0x0010
#define		APIS	0x0020
#define		LOG		0x0040
#define		DMS		0x0080
#define		SLB		0x0100
#define     RAS     0x0200
#define     NTP     0x0400
#define     NRS     0x0800
#define     DS      0x1000
#define     SECS    0x2000
#define     DSS     0x4000
#define     HIDS	0x8000

//DVMM id sise
#define RS_DVMM_ID_SIZE 50

class Exception {
protected:
	string _message;
	//TODO: errorCode
	int _errorCode;
public:
	Exception() {
		_errorCode = -1;
	}

	Exception(const string& message) {
		_message = message;
	}

	virtual ~Exception() {
	}

	virtual const string& message() {
		return _message;
	}
	virtual const int& errorCode() {
			return _errorCode;
	}
};

class RuntimeException : public Exception {
public:
	RuntimeException() {
	}

	RuntimeException(const string& message, const int& errorCode) {
		_message = message;
		_errorCode = errorCode;
	}
};

class CriteriaObject {
protected:
	 enum OBJECT_TYPE {OBJECT, BOOLEAN, SHORT, DOUBLE, LONG, INTEGER, DATE, STRING, OBJECT_LIST};
	int type;
public:
	CriteriaObject() {
		type = OBJECT;
	}
	virtual ~CriteriaObject(){}
	virtual bool empty() {
		return false;
	}
	virtual CriteriaObject* clone(){
		CriteriaObject* newobj = new CriteriaObject();
		return newobj;
	}
	virtual string toString() {
		return string("");
	}
};

class CriteriaBoolean : public CriteriaObject {
	bool value;
public:
	CriteriaBoolean() {
		type = BOOLEAN;
		value = false;
	}

	CriteriaBoolean(bool v) {
		type = LONG;
		value = v;
	}

	~CriteriaBoolean() {
	}

	bool empty() {
		return false;
	}

	virtual CriteriaBoolean* clone() {
		CriteriaBoolean* newobj = new CriteriaBoolean(value);
		return newobj;
	}

	string toString() {
		if (value)
			return string("true");
		else
			return string("false");
	}
};

class CriteriaDouble : public CriteriaObject {
	double value;
public:
	CriteriaDouble() {
		type = DOUBLE;
		value = 0;
	}

	CriteriaDouble(double v) {
		type = DOUBLE;
		value = v;
	}

	~CriteriaDouble() {
	}

	bool empty() {
		return false;
	}

	virtual CriteriaDouble* clone() {
		CriteriaDouble* newobj = new CriteriaDouble(value);
		return newobj;
	}

	string toString() {
		char tmp[512];
		sprintf(tmp, "%f", value);
		return string(tmp);
	}
};

class CriteriaLong : public CriteriaObject {
	long value;
public:
	CriteriaLong() {
		type = LONG;
		value = 0;
	}

	CriteriaLong(long v) {
		type = LONG;
		value = v;
	}

	~CriteriaLong() {
	}

	bool empty() {
		return false;
	}

	virtual CriteriaLong* clone() {
		 CriteriaLong* newobj = new CriteriaLong(value);
		return newobj;
	}

	string toString() {
		char tmp[512];
		sprintf(tmp, "%ld", value);
		return string(tmp);
	}
};

class CriteriaShort : public CriteriaObject {
	int value;
public:
	CriteriaShort() {
		type = SHORT;
		value = 0;
	}

	CriteriaShort(short v) {
		type = INTEGER;
		value = v;
	}

	~CriteriaShort() {
	}

	bool empty() {
		return false;
	}

	virtual CriteriaShort* clone() {
		CriteriaShort* newobj = new CriteriaShort(value);
		return newobj;
	}

	string toString() {
		char tmp[512];
		sprintf(tmp, "%d", value);
		return string(tmp);
	}
};

class CriteriaInteger : public CriteriaObject {
	int value;
public:
	CriteriaInteger() {
		type = INTEGER;
		value = 0;
	}

	CriteriaInteger(int v) {
		type = INTEGER;
		value = v;
	}

	~CriteriaInteger() {
	}

	bool empty() {
		return false;
	}

	virtual CriteriaInteger* clone() {
		 CriteriaInteger* newobj = new CriteriaInteger(value);
		return newobj;
	}

	string toString() {
		char tmp[512];
		sprintf(tmp, "%d", value);
		return string(tmp);
	}
};

class CriteriaString : public CriteriaObject {
	string value;
public:
	CriteriaString() {
		type = STRING;
		value = "";
	}

	CriteriaString(string v, bool bAddQuote = false) {
		type = STRING;
		if (bAddQuote)
			value = string("'") + v + string("'");
		else
			value = v;
	}

	~CriteriaString() {
	}

	bool empty() {
		return (value.length() == 0);
	}

	virtual CriteriaString* clone() {
		 CriteriaString* newobj = new CriteriaString(value);
		return newobj;
	}

	string toString() {
		return value;
	}
};

class CriteriaDate : public CriteriaObject {
	//	int year;
	//	int month;
	//	int day;
	//	int hour;
	//	int minute;
	//	int second;
	//	int msecond;
		string value;
public:
	CriteriaDate() {
		type = DATE;
		value.clear();
//		year = 0;
//		month = 0;
//		day = 0;
//		hour = 0;
//		minute = 0;
//		second = 0;
//		msecond = 0;
	}
	CriteriaDate(string v) {
		type = DATE;
		value = v;
	}

	~CriteriaDate() {
	}

	bool empty() {
		return false;
	}

	virtual CriteriaDate* clone() {
		CriteriaDate* newobj = new CriteriaDate(value);
//		this->year = year;
//		this->month = month;
//		this->day = day;
//		this->hour = hour;
//		this->minute = minute;
//		this->second = second;
//		this->msecond = msecond;
		return newobj;
	}

	string toString() {
//		char tmp[1024];
//		sprintf(tmp, "%d:%d:%d:%d:%d:%d:%d", year, month, day, hour, minute, second, msecond);
//		return string(tmp);
		return value;
	}
};

class CriteriaObjectList : public CriteriaObject{
public:
	list<CriteriaObject*> value;
	CriteriaObjectList() {
		type = OBJECT_LIST;
	}
	virtual
	~CriteriaObjectList() {
		list<CriteriaObject*>::iterator it = value.begin();
		while (it != value.end()){
			if (*it != NULL){
				delete (*it);
			}
			it++;
		}
		value.clear();
	}

	bool empty(){
		return (value.size() == 0);
	}

	void push_back(CriteriaObject& obj) {
		CriteriaObject* newobj = obj.clone();
		value.push_back(newobj);
	}

	int size() {
		return value.size();
	}

	virtual CriteriaObjectList* clone() {
		 CriteriaObjectList* newobjlist = new CriteriaObjectList();
		list<CriteriaObject*>::iterator it = value.begin();
		while (it != value.end()){
			newobjlist->push_back(*(*it));
			it++;
		}
		return newobjlist;
	}

	string toString() {
		string tmp;
		list<CriteriaObject*>::iterator it = value.begin();
		while (it != value.end()){
			tmp.append((*it)->toString());
			it++;
		}
		return tmp;
	}
};

class CCMAObjectID{
	private:
		long id;
		int type;
		int version;
		char obuf[32];
	public:
		CCMAObjectID(long id, int type, int version)
		{
			this->id = id;
			this->type = type;
			this->version = version;
		}
		CCMAObjectID()
		{
			memset(obuf, '\0', sizeof(obuf));
			this->type = 0;
			this->version = 0;
		}
		~CCMAObjectID(){}
		long getId() const{
			return this->id;
		}
		int getType() const{
			return this->type;
		}
		int getVersion() const{
			return this->version;
		}
		void setVersion(const int version) {
			this->version = version;
		}
		bool operator==(const CCMAObjectID & ccma) const{
			return this->id == ccma.id && this->type == ccma.type && this->version == ccma.version;
		}
};

class Record{
	public:
		Record(){};
		virtual ~Record(){};
};

struct CCMAID: public std::binary_function<CCMAObjectID, CCMAObjectID, bool> {
  bool operator () ( const CCMAObjectID &ccma, const CCMAObjectID &id) const {
    return ccma == id;
    }
};

#endif /* CCMA_H_ */

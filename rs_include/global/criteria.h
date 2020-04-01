#ifndef _CRITERIA_H_
#define _CRITERIA_H_

#include "ccma.h"

class Criteria {
protected:
	vector<Criteria*> criteriaOrList;
	list<string> criteriaWithoutValue;
	list<map<string, CriteriaObject*>*> criteriaWithSingleValue;
	list<map<string, CriteriaObject*>*> criteriaWithListValue;
	list<map<string, CriteriaObject*>*> criteriaWithBetweenValue;
	//force sql syntax mode
	string sql_syntax;
	bool sql_syntax_mode;

	string getString(list<string>& mylist) {
		string tmp;
		list<string>::iterator it = mylist.begin();
		while(it != mylist.end()){
			if (it != mylist.begin()) {
				tmp.append("and ");
			}
			tmp.append(*it);
			tmp.append(" ");
			it++;
		}
		return tmp;
	}

	string getString(list<map<string, CriteriaObject*>*>& mylist) {
		string tmp;
		list<map<string, CriteriaObject*>*>::iterator listIt = mylist.begin();
		while (listIt != mylist.end()) {
			map<string, CriteriaObject*>* mapItem = *listIt;
			if (mapItem != NULL){
				if (listIt != mylist.begin()) {
					tmp.append("and ");
				}
				map<string, CriteriaObject*>::iterator it = mapItem->begin();
				while (it != mapItem->end()){
					if (it->second != NULL){
						tmp.append(it->second->toString());
						tmp.append(" ");
					}
					it++;
				}
			}
			listIt++;
		}
		return tmp;
	}

public:
	Criteria() {
		clearSQLCommand();
		clear();
	}
	virtual ~Criteria() {
		clearSQLCommand();
		clear();
	}

	void clear() {
		//release criteriaWithoutValue memory
		criteriaWithoutValue.clear();
		//release criteriaWithSingleValue memory
		list<map<string, CriteriaObject*>*>::iterator listIt = criteriaWithSingleValue.begin();
		while (listIt != criteriaWithSingleValue.end()) {
			map<string, CriteriaObject*>* mapItem = *listIt;
			listIt++;
			if (mapItem != NULL){
				map<string, CriteriaObject*>::iterator it = mapItem->begin();
				while (it != mapItem->end()){
					if (it->second != NULL){
						delete it->second;
						it->second = NULL;
					}
					it++;
				}
				mapItem->clear();
				delete mapItem;
			}
		}
		criteriaWithSingleValue.clear();
		//release criteriaWithListValue memory
		listIt = criteriaWithListValue.begin();
		while (listIt != criteriaWithListValue.end()) {
			map<string, CriteriaObject*>* mapItem = *listIt;
			listIt++;
			if (mapItem != NULL){
				map<string, CriteriaObject*>::iterator it = mapItem->begin();
				while (it != mapItem->end()){
					if (it->second != NULL){
						delete it->second;
						it->second = NULL;
					}
					it++;
				}
				mapItem->clear();
				delete mapItem;
			}
		}
		criteriaWithListValue.clear();
		//release criteriaWithListValue memory
		listIt = criteriaWithBetweenValue.begin();
		while (listIt != criteriaWithBetweenValue.end()) {
			map<string, CriteriaObject*>* mapItem = *listIt;
			listIt++;
			if (mapItem != NULL){
				map<string, CriteriaObject*>::iterator it = mapItem->begin();
				while (it != mapItem->end()){
					if (it->second != NULL){
						delete it->second;
						it->second = NULL;
					}
					it++;
				}
				mapItem->clear();
				delete mapItem;
			}
		}
		criteriaWithBetweenValue.clear();

		//or list
		criteriaOrList.clear();
	}

	bool isValid() {
		return criteriaWithoutValue.size() > 0
			|| criteriaWithSingleValue.size() > 0
			|| criteriaWithListValue.size() > 0
			|| criteriaWithBetweenValue.size() > 0;
	}

	list<string>& getCriteriaWithoutValue() {
		return criteriaWithoutValue;
	}

	list<map<string, CriteriaObject*>*>& getCriteriaWithSingleValue() {
		return criteriaWithSingleValue;
	}

	list<map<string, CriteriaObject*>*>& getCriteriaWithListValue() {
		return criteriaWithListValue;
	}

	list<map<string, CriteriaObject*>*>& getCriteriaWithBetweenValue() {
		return criteriaWithBetweenValue;
	}

	string getCriteriaWithoutValueString() {
		return getString(criteriaWithoutValue);
	}

	string getCriteriaWithSingleValueString() {
		return getString(criteriaWithSingleValue);
	}

	string getCriteriaWithListValueString() {
		return getString(criteriaWithListValue);
	}

	string getCriteriaWithBetweenValueString() {
		return getString(criteriaWithBetweenValue);
	}

	string getCriteriaString(){
		string tmp;
		int size = (int)criteriaOrList.size();
		if (size > 0) {
			tmp.append("(");
			tmp.append(getCriteriaWithoutValueString());
			tmp.append(getCriteriaWithSingleValueString());
			tmp.append(getCriteriaWithListValueString());
			tmp.append(getCriteriaWithBetweenValueString());
			for (int i=0; i<size; i++) {
				Criteria* criteria = criteriaOrList[i];
				if (criteria != NULL) {
					tmp.append(" or ");
					tmp.append(criteria->getCriteriaWithoutValueString());
					tmp.append(criteria->getCriteriaWithSingleValueString());
					tmp.append(criteria->getCriteriaWithListValueString());
					tmp.append(criteria->getCriteriaWithBetweenValueString());
				}
			}
			tmp.append(")");
		} else {
			tmp.append(getCriteriaWithoutValueString());
			tmp.append(getCriteriaWithSingleValueString());
			tmp.append(getCriteriaWithListValueString());
			tmp.append(getCriteriaWithBetweenValueString());
		}
		return tmp;
	}

	void orCriteria(Criteria& criteria) {
		Criteria* criteriaPtr = &criteria;
		criteriaOrList.push_back(criteriaPtr);
	}

	void addCriterion(string condition) {
		if (condition.empty()) {
			throw RuntimeException(string("Value for condition cannot be null"), CCMAErrorCode_criteria_addCriterion1);
		}
		criteriaWithoutValue.push_back(condition);
	}

	void addCriterion(string condition, CriteriaObject& value, string property) {
		if (value.empty()) {
			throw RuntimeException(string("Value for ") + property + string(" cannot be null"), CCMAErrorCode_criteria_addCriterion2);
		}
		 map<string, CriteriaObject*>* objmap = new map<string, CriteriaObject*>();
		 CriteriaObject* newc = new CriteriaString(condition);
		 objmap->insert(pair<string, CriteriaObject*>(string("condition"), newc));
		 CriteriaObject* newv = value.clone();
		 objmap->insert(pair<string, CriteriaObject*>(string("value"), newv));
		 criteriaWithSingleValue.push_back(objmap);
	}

	void addCriterion(string condition, CriteriaObjectList& values, string property) {
		if (values.empty()) {
			throw RuntimeException(string("Value list for ") + property + string(" cannot be null or empty"), CCMAErrorCode_criteria_addCriterion3);
		}
		 map<string, CriteriaObject*>* objmap = new map<string, CriteriaObject*>();
		 CriteriaObject* newc = new CriteriaString(condition);
		 objmap->insert(pair<string, CriteriaObject*>(string("condition"), newc));
		 CriteriaObject* newv = values.clone();
		 objmap->insert(pair<string, CriteriaObject*>(string("values"), newv));
		 criteriaWithListValue.push_back(objmap);
	}

	void addCriterion(string condition, CriteriaObject& value1, CriteriaObject& value2, string property) {
		if (value1.empty() || value2.empty()) {
			throw RuntimeException(string("Between values for ") + property + string(" cannot be null"), CCMAErrorCode_criteria_addCriterion4);
		}
		 map<string, CriteriaObject*>* objmap = new map<string, CriteriaObject*>();
		 CriteriaObject* newc = new CriteriaString(condition);
		 objmap->insert(pair<string, CriteriaObject*>("condition", newc));
		 CriteriaObjectList* newv = new CriteriaObjectList();
		 newv->push_back(value1);
		 newv->push_back(value2);
		 objmap->insert(pair<string, CriteriaObject*>("values", newv));
		 criteriaWithBetweenValue.push_back(objmap);
	}

	//force sql syntax mode
	void setSQLCommand(string query)
	{
		sql_syntax = query;
		sql_syntax_mode = true;
	}
	string getSQLCommand()
	{
		return sql_syntax;
	}
	void clearSQLCommand()
	{
		sql_syntax = "";
		sql_syntax_mode = false;
	}
	bool isSQLCommandMode()
	{
		return sql_syntax_mode;
	}
};

#endif

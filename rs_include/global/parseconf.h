/*
 * parseconf.h
 *
 *  Created on: Feb 8, 2011
 *      Author: root
 */

#ifndef PARSECONF_H_
#define PARSECONF_H_

#include <sstream>
#include <string>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;
extern const char* __progname;

class RSConfig {
	map<string, string> mypairs;

	// ./__progname.conf > ENV($RS) > HOME/.rs.conf > /etc/rs.conf > error
	bool findConfig(){
		bool bFind = false;
		// ./__progname.conf
		string exename = string("./");
		exename.append(__progname);
		exename.append(".conf");
		bFind = parse((char*)exename.c_str());
		if (bFind) return bFind;
		// ENV($RS)
		char* file = getenv("RS");
		if (file != NULL){
			bFind = parse(file);
			if (bFind) return bFind;
		}
		// HOME/.rs.conf
		char* home = getenv("HOME");
		if (home != NULL){
			string mystr = string(home);
			int len = strlen(home);
			if (home[len-1] == '/')
				mystr.append(".rs.conf");
			else
				mystr.append("/.rs.conf");
			bFind = parse((char*)mystr.c_str());
			if (bFind) return bFind;
		}
		// /etc/rs.conf
		char* etc = "/etc/rs.conf";
		bFind = parse(etc);
		if (bFind) return bFind;
		// error
		return false;
	}
public:
	bool parse(void){
		return findConfig();
	}
	string filename;
	bool parse(char* file){
		filename = file;
		FILE* fp;
		fp = fopen(file, "rb");
		if (fp == NULL){
			printf("file %s does not exist.\n", file);
			return false;
		}
		mypairs.clear();
#define SIZE (256)
		char mystring[SIZE];
		char* mystr = NULL;
		char* cmt = NULL;
		int checkcase = 0;
		string addstring;
		string::size_type pos;
		do{
			memset(mystring, 0, SIZE);
			mystr = fgets(mystring, SIZE, fp);
			//skip comment
			if (mystr != NULL){
				cmt = strchr(mystr, '#');
				if (cmt != NULL){
					int len = strlen(cmt);
					memset(cmt, 0, len);
					len = strlen(mystr);
					if (len <= 0) continue;
				}
				if (strcmp(mystr, "\n") == 0) continue;
			}
			checkcase = 0;
			if (mystr != NULL){
				//key:=value
				if (strstr(mystr, "+=") != NULL) {
					checkcase = 2;
				}
				//key=value
				else if (strchr(mystr, '=') != NULL){
					checkcase = 1;
				}
			}

			if (checkcase == 1){
				addstring = string(mystr);
				pos = addstring.find("=");
				if (pos != std::string::npos){
					string key = addstring.substr(0, pos);
					string value = addstring.substr(pos+1, addstring.length()-1-key.length());
					//trim
					pos = key.find_first_not_of(" \t\r\n");
					if (pos != std::string::npos){
						key.erase(0,pos);
					}
					pos = key.find_last_not_of(" \t\r\n");
					if (pos != std::string::npos){
						key.erase(pos+1);
					}
					pos = value.find_first_not_of(" \t\r\n");
					if (pos != std::string::npos){
						value.erase(0,pos);
					}
					pos = value.find_last_not_of(" \t\r\n");
					if (pos != std::string::npos){
						value.erase(pos+1);
					}
					mypairs.insert(pair<string,string>(key,value));
				}// pos != npos
			}// case 1
			//key:=value
			else if (checkcase == 2){
				addstring = string(mystr);
				pos = addstring.find("+=");
				if (pos != std::string::npos){
					string key = addstring.substr(0, pos);
					string value = addstring.substr(pos+2, addstring.length()-2-key.length());
					//trim
					pos = key.find_first_not_of(" \t\r\n");
					if (pos != std::string::npos){
						key.erase(0,pos);
					}
					pos = key.find_last_not_of(" \t\r\n");
					if (pos != std::string::npos){
						key.erase(pos+1);
					}
					pos = value.find_first_not_of(" \t\r\n");
					if (pos != std::string::npos){
						value.erase(0,pos);
					}
					pos = value.find_last_not_of(" \t\r\n");
					if (pos != std::string::npos){
						value.erase(pos+1);
					}
					appendValueString((char*)key.c_str(), (char*)value.c_str());
				}// pos != npos
			}// case 2
		}while(mystr != NULL);
		fclose (fp);
		return true;
	}// parse

	void appendValueString(char* key, char* value){
		map<string, string>::iterator it = mypairs.begin();
		while (it != mypairs.end()){
			if (it->first.compare(key) == 0){
				it->second.append(value);
				return;
			}
			it++;
		}
		string keystr = string(key);
		string valuestr = string(value);
		mypairs.insert(pair<string,string>(keystr,valuestr));
	}

	void print(void){
		map<string, string>::iterator it = mypairs.begin();
		while (it != mypairs.end()){
			printf("%s=%s\n", it->first.c_str(), it->second.c_str());
			it++;
		}
	}

	string getValueString(char* key){
		map<string, string>::iterator it = mypairs.begin();
		while (it != mypairs.end()){
			if (it->first.compare(key) == 0){
				return it->second;
			}
			it++;
		}
		return string("");
	}
	int getValueInt(char* key){
		string mystring = getValueString(key);
		if (mystring.compare("") == 0) return -1;
		stringstream ss;
		ss << mystring;
		int value = -1;
		ss >> value;
		return value;
	}
	unsigned int getValueUnsignedInt(char* key){
		string mystring = getValueString(key);
		if (mystring.compare("") == 0) return 0;
		stringstream ss;
		ss << mystring;
		unsigned int value = 0;
		ss >> value;
		return value;
	}
	long getValueLong(char* key){
		string mystring = getValueString(key);
		if (mystring.compare("") == 0) return -1;
		stringstream ss;
		ss << mystring;
		long value = -1;
		ss >> value;
		return value;
	}
	string toLower(string src){
		string dest = src;
		int len = (int)dest.length();
		for (int i=0; i<len; i++){
			if ('Z' >= dest[i] && dest[i] >= 'A'){
				dest[i] = 'a' + ('Z' - dest[i]);
			}
		}
		return dest;
	}
	bool getValueBool(char* key){
		string mystring = toLower(getValueString(key));
		if (mystring.compare("") == 0) return false;
		if (mystring.compare("true") == 0) return true;
		if (mystring.compare("false") == 0) return false;
		stringstream ss;
		ss << mystring;
		bool value = false;
		ss >> value;
		return value;
	}
};

#endif /* PARSECONF_H_ */

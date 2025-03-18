// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_XTRACE_XTRACE_H_
#define THIRD_PARTY_XTRACE_XTRACE_H_

#include "third_party/rapidjson/include/rapidjson/document.h"
#include "third_party/rapidjson/include/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/include/rapidjson/writer.h"

#ifndef XTRACE_LOCAL_RUN
#include "base/no_destructor.h"
#endif

#include <fstream>
#include <iostream>
#include <random>
#include <map>
#include <string>
#include <vector>

// #define ALLOW_DISCOURAGED_TYPE(x)

namespace blink {

static int xt_id_cnt = 0;

inline std::string generateRandomGuid() {
  std::random_device rd;
  // std::mt19937 mt(rd());
  // If env has "XTRACE_SEED" set, use that as seed else use random
  static char* seed_env = std::getenv("XTRACE_SEED_0");

  // If seed_env is set, return a counted string
  // else return a random string
  if(seed_env) {
    xt_id_cnt++;
    // Convert id_cnt to string
    std::string id_cnt_str = std::to_string(xt_id_cnt);
    return "ID_" + id_cnt_str;
  }else{
    std::cout << "Using random seed" << std::endl;
  }

  static std::mt19937 mt(rd());
  std::uniform_int_distribution<int> dist(0, 15);

  const char *v = "0123456789abcdef";
  std::string res;

  for (int i = 0; i < 32; i++) {
    if (i == 8 || i == 12 || i == 16 || i == 20)
      res += "-";
    res += v[dist(mt)];
  }

  return res;
}

class XTrace {

public:
  std::string crid;
  int timeCount = 0;
  bool enableJsonFlush = false;

  // std::vector<std::string> events ALLOW_DISCOURAGED_TYPE("Need to use");
  std::vector<std::string> events;

  // Map of name to count
  std::map<std::string, int> run_names_count_map;

  // static XTrace *instance;

  // static XTrace *getInstance();

  inline static XTrace *getInstance() {
#ifndef XTRACE_LOCAL_RUN
    static base::NoDestructor<XTrace> monitor;
    return monitor.get();
#else
    static XTrace monitor;
    return &monitor;
#endif
    // static XTrace instance; // Thread-safe in C++11 and later
    // return &instance;
    // // If the instance doesn't exist, create it
    // if (!instance) {
    //   instance = new XTrace();
    // }
    // return instance;
  }

  inline XTrace() {
    this->crid = generateRandomGuid();
    std::cout << "XTrace constructor" << std::endl;
    ResetCodeRunId("Start");
  }

  inline ~XTrace() { std::cout << "XTrace destructor" << std::endl; }

  inline void ResetCodeRunId(std::string name){
    // Get xTrace_Prefix from environment variable "XTRACE_PREFIX"
    const char* xTrace_Prefix_cstr = std::getenv("XTRACE_PREFIX");
    std::string xTrace_Prefix = xTrace_Prefix_cstr ? std::string(xTrace_Prefix_cstr) : "";

    // Add to run_names_count_map count
    if (run_names_count_map.find(name) == run_names_count_map.end()) {
      run_names_count_map[name] = 0;
    } else {
      run_names_count_map[name] += 1;
    }

    int count = run_names_count_map[name];
    name = xTrace_Prefix + "/" + name + "/" + std::to_string(count);

    std::string eventType = "NEW_CODE_RUN_ID";
    crid = generateRandomGuid();

    std::vector<std::string> payload;
    payload.push_back(name);
    DispatchEvent(eventType, getVectorOfStringToJson(payload));
  }

  inline void LocalVarUpdate(std::string mrid, std::string varName, std::string varValue) {

    int timeStamp = timeCount;
    timeCount++;
    SendVarUpdate(mrid, varName, true, "", varValue, timeStamp);
  }

  inline void SendVarUpdate(std::string contId, std::string varName, bool isLocal, std::string className, std::string newVal, int timeStamp) {
    std::string eventType = "SEND_VAR_UPDATE";
    std::string newValStr = newVal;
    std::string varType = "VALUE";

    std::vector<std::string> payload;
    payload.push_back(contId);
    payload.push_back(varName);
    payload.push_back(varType);
    payload.push_back(className);
    payload.push_back(newValStr);
    payload.push_back(std::to_string(timeStamp));
    payload.push_back(isLocal ? "true" : "false");

    DispatchEvent(eventType, getVectorOfStringToJson(payload));
}

  inline void LogLineRun(std::string mrid, int lineNo) {

    std::string type = "LINE_EXEC";
    std::vector<std::string> payload;

    int timeStamp = timeCount;
    timeCount++;

    payload.push_back(mrid);
    payload.push_back(std::to_string(lineNo));
    payload.push_back(std::to_string(timeStamp));
    payload.push_back("");

    std::string payload_str = getVectorOfStringToJson(payload);
    DispatchEvent(type, payload_str);
  }

  inline std::string OnMethodEnter(std::string relativeFilePath,
                            std::string methodName, std::string codeVersion) {

    std::string mrid = generateRandomGuid();
    std::cout << "OnMethodEnter called" << std::endl;

    std::string type = "METHOD_ENTER";

    std::vector<std::string> payload;
    payload.push_back(mrid);
    payload.push_back(relativeFilePath);
    payload.push_back(methodName);
    payload.push_back("dummy_threadId");
    payload.push_back("dummy_clrid");
    payload.push_back(codeVersion);

    std::string payload_str = getVectorOfStringToJson(payload);
    DispatchEvent(type, payload_str);
    return mrid;
  }

  inline void DispatchEvent(std::string eventType, std::string payload) {
    std::vector<std::string> vec;
    vec.push_back(crid);
    vec.push_back(eventType);
    vec.push_back(payload);

    std::string msg = getVectorOfStringToJson(vec);
    events.push_back(msg);
    WriteStringToJsonLOGFile(msg);
  }

  inline void WriteStringToJsonLOGFile(std::string msg) {
    std::cout << "WriteStringToJsonLOGFile called" << std::endl;
    std::ofstream ofs;
    ofs.open("xtrace.run.log", std::ofstream::out | std::ofstream::app);

    if (!ofs.is_open()) {
        std::cout << "Failed to open file for writing: xtrace.run.log" << std::endl;
        return;
    }

    ofs << msg << std::endl;
    ofs.flush(); // Ensure the log is written immediately

    if (ofs.fail()) {
        std::cout << "Failed to write to file: xtrace.run.log" << std::endl;
    }

    ofs.close();
    std::cout << "Data written" << std::endl;

  }

  inline std::string getVectorOfStringToJson(std::vector<std::string> &vec) {

    rapidjson::Document d;
    d.SetArray();

    rapidjson::Value array(rapidjson::kArrayType);

    for (auto it = vec.begin(); it != vec.end(); ++it) {
      rapidjson::Value str(it->c_str(), d.GetAllocator());
      d.PushBack(str, d.GetAllocator());
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);

    return buffer.GetString();
  }

  inline void FlushAllEventsToJSONFile() {

    if(!enableJsonFlush) {
      return;
    }
    std::cout << "Flushing events to json" << std::endl;
    std::string json = getVectorOfStringToJson(events);

    std::cout << json;

    std::ofstream ofs;
    ofs.open("xtrace.run.json", std::ofstream::out | std::ofstream::trunc);

    if (!ofs.is_open()) {
        std::cout << "Failed to open file for writing: xtrace.run.json" << std::endl;
        return;
    }
    ofs << json;
        if (ofs.fail()) {
        std::cout << "Failed to write to file: xtrace.run.json" << std::endl;
    }
    ofs.close();
  }
};


} // namespace blink

#endif // THIRD_PARTY_XTRACE_XTRACE_H_

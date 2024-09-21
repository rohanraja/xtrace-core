// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_XTRACE_XTRACE_H_
#define THIRD_PARTY_XTRACE_XTRACE_H_

#include "third_party/rapidjson/include/rapidjson/document.h"
#include "third_party/rapidjson/include/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/include/rapidjson/writer.h"
#include "base/no_destructor.h"

#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// #define ALLOW_DISCOURAGED_TYPE(x)

namespace blink {

inline std::string generateRandomGuid() {
  std::random_device rd;
  std::mt19937 mt(rd());
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

  // std::vector<std::string> events ALLOW_DISCOURAGED_TYPE("Need to use");
  std::vector<std::string> events;

  // static XTrace *instance;

  // static XTrace *getInstance();

  inline static XTrace *getInstance() {
    static base::NoDestructor<XTrace> monitor;
    return monitor.get();
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
    std::cout << "XTrace constructor again 2" << std::endl;
  }

  inline ~XTrace() { std::cout << "XTrace destructor" << std::endl; }

  inline void ResetCodeRunId(std::string name){
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

    std::vector<std::string> methods_to_trace = {
        "Clipboard::write", 
        "Clipboard::read", 
        "Clipboard::readText", 
        "Clipboard::writeText"
    };
    if(methods_to_trace.size() > 0){
      bool found = false;
      for (auto & it : methods_to_trace) {
        // Check if methodName exists in the vector
        if (methodName.find(it) != std::string::npos) {
          found = true;
          break;
        }
      }
      if(found){
        ResetCodeRunId(methodName);
      }
    }
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

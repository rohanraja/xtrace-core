/// #include
int sum(int a, int b) {
    /// XTrace::getInstance
    /// OnMethodEnter
    /// sum, method_args, .cc
    /// SendVarUpdate, a, std::to_string
    /// SendVarUpdate, b, std::to_string
    /// LogLineRun
    /// FlushAllEventsToJSONFile
  return a + b;
}

void print(int a) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
    /// print, method_args, .cc
  xtrace->SendVarUpdate(xtrace_mrid, "a", std::to_string(a))
      xtrace->LogLineRun(xtrace_mrid, 5);
  xtrace->FlushAllEventsToJSONFile();
  cout << a;
}

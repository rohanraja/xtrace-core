using System;
using System.Collections.Generic;

namespace HooksInjectorCommon.Entities.NodePropertieClasses
{
	public class StatementProperties : INodeProperties
    {
		public bool IsLocalVarDec = false;
		public bool IsVarChanger = false;
		public List<string> localVars = new List<string>(){};
        public string MethodNameCalled = "";

     
		public StatementProperties(int lineNo)
        {
			LineNo = lineNo;
		}

		public int LineNo { get; }
	}
}

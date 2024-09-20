using System;
namespace HooksInjectorCommon.Entities
{
	public class SyntaxStringNode : ISyntaxStringNode
    {
		string _codeString = "";

        public SyntaxStringNode(string codeStr)
        {
			_codeString = codeStr;
        }

		public string CodeString => _codeString;
	}
}

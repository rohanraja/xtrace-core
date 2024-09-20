using System;
using System.Collections.Generic;

namespace HooksInjectorCommon.Entities.NodePropertieClasses
{
	public class FunctionLikeNodeProperties : INodeProperties
    {
		public FunctionLikeNodeProperties (string name, List<ISyntaxNode> statements)
        {
			Name = name;
			Statements = statements;
		}

		public string Name { get; }
		public List<ISyntaxNode> Statements { get; }
	}
}

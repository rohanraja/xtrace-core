using System;
using System.Collections.Generic;

namespace HooksInjectorCommon.Entities
{
	public interface ISyntaxNode
	{
		List<ISyntaxNode> Children { get; set; }
		ISyntaxNode Parent { get;  }
		object LibObject { get; }
		SyntaxNodeKind Kind { get; }
		INodeProperties Properties { get; set; }
		string Id { get; }
	}
}

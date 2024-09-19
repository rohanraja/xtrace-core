using System;
using System.Collections.Generic;

namespace HooksInjectorCommon.Entities
{
	public class SyntaxNode : ISyntaxNode
    {
		private List<ISyntaxNode> _children = new List<ISyntaxNode>(){};
		private ISyntaxNode _parent = null;
		private object _libObject;
		private SyntaxNodeKind syntaxNodeKind;
		private INodeProperties properties;
		private string _id = "";

		private static int syntaxNodesCount = 0;

		public SyntaxNode(object libObject, SyntaxNodeKind syntaxNodeKind, string text)
        {
			_libObject = libObject;
			this.syntaxNodeKind = syntaxNodeKind;
			Text = text;
			syntaxNodesCount++;
			this._id = syntaxNodesCount.ToString();
		}


		public ISyntaxNode Parent => _parent;

		public object LibObject => _libObject;


		public SyntaxNodeKind Kind => syntaxNodeKind;

		public List<ISyntaxNode> Children { get => _children; set => _children = value; }

		public INodeProperties Properties { get => properties; set => properties = value; }

		public string Id => _id;

		public string Text { get; }
	}
}

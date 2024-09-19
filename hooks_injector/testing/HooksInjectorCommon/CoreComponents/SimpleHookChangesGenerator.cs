using System;
using System.Collections.Generic;
using HooksInjectorCommon.CoreInterfaces;
using HooksInjectorCommon.Entities;
using HooksInjectorCommon.Entities.NodePropertieClasses;
using HooksInjectorCommon.LangInterfaces;

namespace HooksInjectorCommon.CoreComponents
{
	public class SimpleHookChangesGenerator : IHookChangesGenerator
    {
		private readonly IHookTemplates hookTemplates;
		string filePath = "";
		bool isCSProj = false;

		string currentMethodName = "";

		public SimpleHookChangesGenerator(IHookTemplates hookTemplates = null)
		{
			this.hookTemplates = hookTemplates;
		}

		public void GenerateHookChanges(ISyntaxNode syntaxNode, ITreeChangeInfo treeChangeInfo, string filename)
		{
			filePath = filename;

			WalkTree(syntaxNode, treeChangeInfo);
		}

		private void WalkTree(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
		{
			if (node == null)
				return;

			bool isProcessed = ProcessSyntaxNode(node, treeChangeInfo);
            
			if(!isProcessed)
    			ContinueWalk(node, treeChangeInfo);
		}

		private void ContinueWalk(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
		{
			foreach (var child in node.Children)
			{
				WalkTree(child, treeChangeInfo);
			}
		}
        
		private bool ProcessSyntaxNode(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
		{
			bool processed = false;

			if (node.Kind == SyntaxNodeKind.RootKind)
            {
				ProcessRootNode(node, treeChangeInfo);
                processed = true;
            }

			if(IsCodeRunner(node.Kind))
			{
				ProcessCodeRunnerNode(node, treeChangeInfo);
				processed = true;
			}

			if (IsStatement(node.Kind))
            {
                ProcessStatementNode(node, treeChangeInfo);
				if (node.Kind == SyntaxNodeKind.DirectChildStatement)
					processed = true;
            }
			if (node.Kind == SyntaxNodeKind.ModuleBlock)
				currentMethodName = "module";

			return processed;
		}

		private void ProcessStatementNode(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
		{
			List<ISyntaxStringNode> syntaxNodes = new List<ISyntaxStringNode>() { };

			var statementProps = node.Properties as StatementProperties;

			string logLineRun = hookTemplates.LogLineRunHook(statementProps.LineNo, statementProps.MethodNameCalled);

			if (currentMethodName.ToLower().Contains("constructor") && (node as SyntaxNode).Text.Contains("super"))
				return;

			if (currentMethodName == "module")
				return;
			//if ((node as SyntaxNode).Text.Contains("use strict"))
				//return;
			
			syntaxNodes.Add(new SyntaxStringNode(logLineRun));

			treeChangeInfo.AddNodesBefore(node, syntaxNodes);
		}

		private void ProcessCodeRunnerNode(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
		{
			FunctionLikeNodeProperties functionDeclarationProperties = node.Properties as FunctionLikeNodeProperties;
			if(functionDeclarationProperties != null)
    			currentMethodName = functionDeclarationProperties.Name;

			List<ISyntaxStringNode> syntaxNodes = new List<ISyntaxStringNode>() { };

			string onMethodEnterLine = hookTemplates.OnMethodEnterHook(this.filePath, this.currentMethodName);
            syntaxNodes.Add(new SyntaxStringNode(onMethodEnterLine));

			if (node.Children.Count == 0)
				return;
			
			if (node.Children[0].Children.Count > 0)
			{
				var firstChild = node.Children[0].Children[0];
				if(currentMethodName.ToLower().Contains("constructor") && (firstChild as SyntaxNode).Text.Contains("super"))
				{
					firstChild = null;
					if (node.Children[0].Children.Count > 1)
						firstChild = node.Children[0].Children[1];
				}
				if(firstChild != null)
					treeChangeInfo.AddNodesBefore(firstChild, syntaxNodes);

			}
			else
				treeChangeInfo.CreateChildNodes(node.Children[0], syntaxNodes);

			ContinueWalk(node, treeChangeInfo);
		}

		private void ProcessRootNode(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
        {
            currentMethodName = "RootMethod";

            List<ISyntaxStringNode> syntaxNodes = new List<ISyntaxStringNode>() { };

			string importHook = hookTemplates.ImportLibHook(filePath);

            string onMethodEnterLine = hookTemplates.OnMethodEnterHook(this.filePath, this.currentMethodName);

			syntaxNodes.Add(new SyntaxStringNode(importHook));
            syntaxNodes.Add(new SyntaxStringNode(onMethodEnterLine));

            if (node.Children.Count > 0 && !isCSProj)
                treeChangeInfo.AddNodesBefore(node.Children[0], syntaxNodes);

            ContinueWalk(node, treeChangeInfo);
        }

		private bool IsStatement(SyntaxNodeKind kind)
        {
			return kind == SyntaxNodeKind.Statement || kind == SyntaxNodeKind.BlockedStatement || kind == SyntaxNodeKind.DirectChildStatement ;
        }

		private bool IsCodeRunner(SyntaxNodeKind kind)
		{
			return kind == SyntaxNodeKind.MethodDeclaration || kind == SyntaxNodeKind.FunctionDeclaration || kind == SyntaxNodeKind.Constructor  ;
		}
	}
}

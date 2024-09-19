using System;
using HooksInjectorCommon.Entities;

namespace HooksInjectorCommon.CoreInterfaces
{
    public interface ITreeBuilder
    {
		void BuildTree(ISyntaxNode root);
    }
}

using System;
using System.Collections.Generic;
using HooksInjectorCommon.Entities;

namespace HooksInjectorCommon.LangInterfaces
{
	public interface INodeInfoCollector
    {
		List<ISyntaxNode> ListChildren (object libObject); 
		INodeProperties CollectProperties (object libObject) ;
    }
}

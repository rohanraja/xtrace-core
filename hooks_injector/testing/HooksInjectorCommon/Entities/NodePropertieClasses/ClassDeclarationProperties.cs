using System;

namespace HooksInjectorCommon.Entities.NodePropertieClasses
{
	public class ClassDeclarationProperties : INodeProperties
    {
		public ClassDeclarationProperties (string name)
        {
			Name = name;
		}

		public string Name { get; }
	}
}

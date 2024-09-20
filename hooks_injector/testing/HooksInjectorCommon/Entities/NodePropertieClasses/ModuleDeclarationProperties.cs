using System;

namespace HooksInjectorCommon.Entities.NodePropertieClasses
{
	public class ModuleDeclarationProperties : INodeProperties
    {
		public ModuleDeclarationProperties (string name)
        {
			Name = name;
		}

		public string Name { get; }
	}
}

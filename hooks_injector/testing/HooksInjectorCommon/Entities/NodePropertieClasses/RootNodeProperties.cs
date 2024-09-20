using System;
namespace HooksInjectorCommon.Entities.NodePropertieClasses
{
	public class RootNodeProperties : INodeProperties
    {
        public RootNodeProperties(string fileName)
        {
			FileName = fileName;
		}

		public string FileName { get; }
	}
}

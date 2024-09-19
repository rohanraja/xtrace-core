using System;
using HooksInjectorCommon.Entities;

namespace HooksInjectorCommon.LangInterfaces
{
    public interface IHookTemplates
    {
		CodeHook GetTemplateForHookType(CodeHookType hookType);

		string OnMethodEnterHook(string filePath, string methodName);
		string LogLineRunHook(int lineNo, string methodNameCalled);
		string ImportLibHook(string filePath);
		string LocalVarUpdateHook(string varName);
	}
}

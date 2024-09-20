using System;
using CodeParserCommon;
using HooksInjectorCommon.CoreComponents;

namespace CppHooksInjector
{
    public class CppSourceFileHooker : ISourceFileHooker
	{
		private static readonly log4net.ILog log = log4net.LogManager.GetLogger(System.Reflection.MethodBase.GetCurrentMethod().DeclaringType);

        private CppParser parser;

		public CppSourceFileHooker()
		{
            this.parser = new CppParser();
		}

		public void AddHooksToSourceCode(SourceCodeInfo sourceCodeInfo, Guid version)
		{
            // Todo - add version to method runs
            foreach (var sourceFile in sourceCodeInfo.SourceFiles)
            {
				try
				{

					string contents = sourceFile.GetCode();
                    string fileName = sourceFile.FilePath;

					string onlyFileName = System.IO.Path.GetFileName(fileName);

					log.InfoFormat("Running CPP HookInjection Pipeline for file: {0}", fileName);
                    string outText = this.parser.Parse(contents, onlyFileName, version);
                    sourceFile.UpdateCodeContents(outText);
				}
				catch(Exception e)
				{
					log.Error("Error in Hook Injection", e);

				}

            }
		}

    }
}
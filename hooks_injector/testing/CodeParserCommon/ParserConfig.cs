using System;
using System.Collections.Generic;

namespace CodeParserCommon
{
    public class ParserConfig
    {

        public ParserConfig()
        {
        }

		public string ReproCsProj { get; set; }
		public string Language { get; set; }
		public string InjectProjReferenceStr { get; set; } = "";
		public List<string> BlackListFiles { get; set; } = new List<string> { };

		public bool InjectProjReference
		{
			get
			{
				if (string.IsNullOrWhiteSpace(InjectProjReferenceStr))
					return true;

				return bool.Parse(InjectProjReferenceStr);
			}
		}


		public ProjectLanguageType getLangType()
		{
			return parseLangTypeString(Language);
		}

		public static ProjectLanguageType parseLangTypeString(string langTypeStr)
        {
            if (langTypeStr.ToLower().Contains("csharp"))
                return ProjectLanguageType.CSharp;

            if (langTypeStr.ToLower().Contains("typescript"))
                return ProjectLanguageType.TypeScript;

            if (langTypeStr.ToLower().Contains("cpp"))
                return ProjectLanguageType.Cpp;

            return ProjectLanguageType.CSharp;
        }
	}
}

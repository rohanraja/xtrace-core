using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace HooksInjectorCommonTests.Helpers
{
    public class CodeAssertHelpers
    {
        public static bool MatchCodeWithHookedCode(string hookedCode, string expectedCode)
        {
            List<string> hookedLines = GetTrimmedLines(hookedCode);
            List<string> expectedLines = GetTrimmedLines(expectedCode);

            // TODO: Create a common class "TmpFile.GenerateTmpFilePath" to generate tmp file paths
			File.WriteAllText("/tmp/code.ts", hookedCode);

            Assert.AreEqual(expectedLines.Count, hookedLines.Count);

            for(int i=0; i<hookedLines.Count; i++)
            {
                string expectedLine = expectedLines[i];
                string hookedLine = hookedLines[i];
                if(expectedLine.StartsWith("///"))
                {
                    List<string> words = GetWordsFromExpectedHookLine(expectedLine);
                    foreach(var word in words)
                    {
						Assert.IsTrue(hookedLine.ToLower().Contains(word.ToLower()), "line - \"{0}\" doesn't contain word \"{1}\"", hookedLine, word);
                    }
                }
                else
                {
                    Assert.AreEqual(expectedLine, hookedLine);
                }
            }

            return true;
        }

        private static List<string> GetWordsFromExpectedHookLine(string expectedLine)
        {
            var outp = new List<string>() { };
            string plainLine = expectedLine.Replace("///", "");

            foreach(var word in plainLine.Split(','))
            {
                var trimmedWord = word.Trim();
                if(!string.IsNullOrWhiteSpace(trimmedWord))
                    outp.Add(trimmedWord);
            }

            return outp;
        }

        public static List<string> GetTrimmedLines(string code)
        {
            var outp = new List<string>() { };

            foreach(var line in code.Split('\n'))
            {
                var trimmedLine = line.Trim();
                if (string.IsNullOrWhiteSpace(trimmedLine))
                    continue;
                if (trimmedLine == "\"")
                    continue;

                outp.Add(trimmedLine);
            }

            return outp;
        }
    }
}

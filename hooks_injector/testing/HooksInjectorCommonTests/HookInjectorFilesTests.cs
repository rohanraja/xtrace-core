using Microsoft.VisualStudio.TestTools.UnitTesting;
using HooksInjectorCommon.Entities;
using HooksInjectorCommon.CoreInterfaces;
using HooksInjectorCommon.CoreComponents;
using HooksInjectorCommon.LangInterfaces;
using System;
using Moq;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.CompilerServices;
using Castle.Core;
using HooksInjectorCommonTests.Helpers;
using CodeParserCommon;
using CppHooksInjector;

namespace HooksInjectorCommonTests
{

    [TestClass]
    public class HookInjectorFilesTests
    {
        [DataTestMethod]
        [DynamicData(nameof(GetCppFolders), DynamicDataSourceType.Method)]
        public void TestCpp(string testFolder)
        {
            string lang = "cpp";
            List<string> exts = new List<string> { ".cc" };
            ISourceFileHooker sourceFileHooker = new CppSourceFileHooker();
            RunHookedTests(lang, exts, sourceFileHooker, testFolder);
        }

        private static void RunHookedTests(string lang, List<string> exts, ISourceFileHooker sourceFileHooker, string dir)
        {
            var files = Directory.GetFiles(dir);

            List<Pair<string, string>> testFiles = new List<Pair<string, string>>();

            string tmpDir = Path.Combine(dir, "tmp");

            if (Directory.Exists(tmpDir))
            {
                Directory.Delete(tmpDir, true);
            }
            Directory.CreateDirectory(tmpDir);

            SourceCodeInfo sourceCodeInfo = new SourceCodeInfo(tmpDir);

            foreach (var expectedFile in files)
            {
                if (!exts.Contains(Path.GetExtension(expectedFile))) continue;

                // Get file name without extension 
                string fileName = Path.GetFileNameWithoutExtension(expectedFile);

                if (fileName.EndsWith(".expected"))
                {
                    string baseFileName = Path.GetFileNameWithoutExtension(fileName);
                    string baseFileNameExt = baseFileName + Path.GetExtension(expectedFile);

                    string fullBaseFileName = Path.Combine(Path.GetDirectoryName(expectedFile), baseFileNameExt);

                    // Copy fullBasdFileName to tmp
                    string newFile = Path.Combine(tmpDir, baseFileNameExt);
                    File.Copy(fullBaseFileName, newFile);
                    sourceCodeInfo.AddCodeFile(baseFileNameExt);
                    testFiles.Add(new Pair<string, string>(newFile, expectedFile));
                }
            }

            // Add hooks to source code
            sourceFileHooker.AddHooksToSourceCode(sourceCodeInfo, Guid.NewGuid());

            // Compare the hooked code with expected code
            foreach (var testFile in testFiles)
            {
                var hookedCode = File.ReadAllText(testFile.First);
                var expectedHookedCode = File.ReadAllText(testFile.Second);
                Debug.WriteLine("Checking file: " + testFile.Second);
                CodeAssertHelpers.MatchCodeWithHookedCode(hookedCode, expectedHookedCode);
            }
        }


        public static IEnumerable<object[]> GetCppFolders()
        {
            return GetTestFolders("cpp");
        }

        public static IEnumerable<object[]> GetTypescriptFolders()
        {
            return GetTestFolders("typescript");
        }

        public static IEnumerable<object[]> GetTestFolders(string lang)
        {
            string sourceCodeDirectory = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            string testFilesDir = Path.Combine(sourceCodeDirectory, "../../../..", lang);

            var dirs = Directory.GetDirectories(testFilesDir);

            foreach (var dir in dirs)
            {
                yield return new object[] { dir };
            }
        }
    }
}

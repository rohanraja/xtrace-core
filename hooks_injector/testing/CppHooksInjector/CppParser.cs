
using System;
using System.Diagnostics;
using System.IO;

namespace CppHooksInjector{
    public class CppParser
    {
        public string Parse(string code, string fileName, Guid codeVersion)
        {

            string exePath = System.Reflection.Assembly.GetExecutingAssembly().Location;
            string pwd = System.IO.Path.GetDirectoryName(exePath);

            string baseCppPath = Path.Join(pwd, "../../../../../");

            string appPath = Path.Join(baseCppPath, "cpp_hooks_injector");

            // Change the current directory to the app path
            // Environment.CurrentDirectory = appPath;
            string output = RunTypeScriptFile("dist/out.js", code, appPath, fileName, codeVersion);
            if(string.IsNullOrEmpty(output))
            {
                return code;
            }
            string formatted = RunCppFormat(output, baseCppPath);
            return formatted;
        }

        static string RunCppFormat(string code, string baseCppPath)
        {
            string formatterPath = Path.Join(baseCppPath, "cpp_formatter/node_modules/.bin/clang-format");
            try
            {
                ProcessStartInfo startInfo = new ProcessStartInfo
                {
                    FileName = formatterPath,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    RedirectStandardInput = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };

                using (Process process = new Process { StartInfo = startInfo })
                {
                    process.Start();
                    // Write the contents to the StandardInput of the process
                    using (StreamWriter writer = process.StandardInput)
                    {
                        writer.Write(code);
                    }

                    string output = process.StandardOutput.ReadToEnd();
                    string error = process.StandardError.ReadToEnd();

                    process.WaitForExit();

                    if (!string.IsNullOrEmpty(error))
                    {
                        throw new Exception($"Error running TypeScript file: {error}");
                    }

                    return output;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("An error occurred: " + ex.Message);
                return string.Empty;
            }
        }


        static string RunTypeScriptFile(string tsFilePath, string code, string dir, string fileName, Guid codeVersion)
        {
            try
            {
                ProcessStartInfo startInfo = new ProcessStartInfo
                {
                    FileName = "node",
                    Arguments = tsFilePath,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    RedirectStandardInput = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    WorkingDirectory=dir
                };

                startInfo.EnvironmentVariables["FileName"] = fileName;
                startInfo.EnvironmentVariables["CodeVersion"] = codeVersion.ToString();

                using (Process process = new Process { StartInfo = startInfo })
                {
                    process.Start();
                    // Write the contents to the StandardInput of the process
                    using (StreamWriter writer = process.StandardInput)
                    {
                        writer.Write(code);
                    }

                    string output = process.StandardOutput.ReadToEnd();
                    string error = process.StandardError.ReadToEnd();

                    process.WaitForExit();

                    if (!string.IsNullOrEmpty(error))
                    {
                        throw new Exception($"Error running TypeScript file: {error}");
                    }

                    return output;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("An error occurred: " + ex.Message);
                return string.Empty;
            }
        }
    }
}
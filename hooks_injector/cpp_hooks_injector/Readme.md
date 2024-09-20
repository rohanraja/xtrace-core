## 1. Clone repo
```shell
git clone https://github.com/rohanraja/xtrace-core
```

## 2. To install and test, run the following commands

```shell
cd hooks_injector/cpp_hooks_injector
npm install
npm install -g clang-format
npm run build
npm run test

# Install clang-formatter module
cd hooks_injector/cpp_formatter
npm install
```

After running the last command, code with xTrace logs should be generated in tests/out.cc.

## 3. To debug in VSCode

1. Put breakpoint in the generated JS code in "dist/out.js"
2. Run "Debug: Javascript Debug Terminal" from VSCode command.
3. Within the terminal run "npm run test"

## 4. To add a new feature, we can follow these steps:

1. Find a test cpp code snippet for your scenario. E.g. if working to add support for class, find a real cpp code which has the class definition.
2. Replace contents of "tests/inp.cc" with your test cpp snippet.
3. Run "npm run test" to generate "tests/out.cc" with current output.
4. Make code changes until expected output is generated.
5. If needed, debug as described in Step 3.)


## 5. For generating code in your local Chromium repo code
1. Open "inject_in_folders.ts" and populate "foldersToInject" array with absolute paths of folders to inject code into (example, "modules/clipboard" folder)
2. Run "npm run folders". This will inject xtrace code into all files of the specified folder.


## 6. For testing if the generated code is getting compiled without errors

1. Ensure your chromium is building correctly
2. Copy "<root>/cpp_recorder/xtrace" folder into "src/third_party" folder so that "third_party/xtrace/xtrace.h" is a valid file
3. Generate xTrace code in your local Chromium repo files as mentioned in Step 5.
4. Build chromium and ensure no compiler errors
5. Run chromium with NO SANDBOX mode (add --no-sandbox arg), perform any scenario which hits clipboard code. Close chrome.
6. In the out/debug_full_x64 folder, a file named "xtrace.run.json" should be created with logs populated


## 7. Running the recordings jsons in xTrace to check how it looks E2E

1. Open xTrace UI here - http://20.244.33.135:3009/
2. Upload code_events.json file from xtrace-core folder
3. Upload xtrace.run.json from CHromium debug out folder. (Ensure this order of upload is followed)

Refresh the site and the first recording in the list will be the one uploaded.
To upload any file, click on "+" icon at the top left of the page 

## 8. Running the unit tests for cpp hooks injector

Unit tests reside in folder - hooks_injector/testing/cpp

Each unit test is a separate folder. E.g. hooks_injector/testing/cpp/simple main is one unit test

Within a unit test, each .cc file has a corresponding .expected.cc file. The expected.cc file provides hints regarding what to expect in the generated code. 
Example: "/// OnMethodEnter", which is starting with "///" indicates that the particular line should contain word "OnMethodEnter".

Ensure that Step #2 is completed sucessfully

Install Dot Net core SDK in your system

```shell
# Install .net core SDK. Ensure 'winget' is available in your system
winget install Microsoft.DotNet.SDK.8

cd hooks_injector/testing/HooksInjectorCommonTests
dotnet test
```

When the tests run, it also runs "clang-format".
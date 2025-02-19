# User guide on using Ramp easy for running custom web tests or custom web scenarios

## Pre-reqs
- Have a chromium enlistment with build working correctly, **ensure git worktree is clean with no active changes!!!**
- Install NodeJS 18.16.0
- VSCode

## 1. Clone repo
```shell
git clone https://github.com/rohanraja/xtrace-core
```
## 2. Install NodeJS
- Any latest version should work, last known working version is 18.16.0

## 3. Open xtrace-core folder in VSCode
- VScode will be the UI to interact with Ramp easy config and running tasks

## 4. Run task - "Install: use"
- Hit "Ctrl+Shift+P" and search for "Run task" command, execute it.
- A list of tasks available are shown, search for the task to run "Install: use" and hit Enter
- If prompted, click "Continue without scanning"

## 5. Run task - "Config.Add new"
- Name the config according to your scenario - the default is a config to understand double click selection flow, name it "Double click"
- The config json opens up, edit the file according to your case, check [this guide](#runconfig-json-file) on run_config.json.
- You can leave it unchanged since the default config is good enough to run the double click selection demo.

## 6. Run task - "Run.E2E"
- **WARNING**: Your git tree will be reset hard, please make sure you don't have any active changes in your src folder.
- This should trigger the complete e2e pipeline to hook code in chromium, compile, run chromium and then upload recording to RampEasy server
- Wait for few minutes for the pipeline to build, should take 5-20 mins
- After sometime, browser should open up with a test page. Try to double click any word. Then close the browser to upload recording.
- Post completion, a url to ramp-easy with the recording will be shown in the output.
- Visit the rampeasy url to view the recording.


## RunConfig json file

- cr_path: The local path to "cr" folder where the chromium enlistment resides
- debug_folder_name: the out folder where the build is generated, e.g. 
    - "debug_x64"
    - "debug_full_x64"
    - "release_arm64"

- files_whitelist: List of files which should be recorded
- methods_blacklist: Ignore a method for recording if methodname includes any of the item in this config.
- methods_which_split_run: Methods from which we want to study code execution
- web_page: Either an html file present in run_configs/html folder or a url starting with http, which should be opened to perform user action.
- web_test: Relative path of the webtest that should be run to record code


## Troubleshoot

- To view logs file for your last run, run task "Logs.Open last run"

### Code not compiling

Most of the times, failures would be caused by parsing of C++ code which is not yet supported. In those cases, there are 2 options:

1. Blacklist the function from being recorded - use "methods_blacklist" run config
2. Blacklist the file from being recorded - use 

### Code recording not uploading

Mostly the Ramp Easy server is down. Contact Rohan Raja (@roraja).
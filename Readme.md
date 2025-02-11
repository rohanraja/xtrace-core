# User guide on using xTrace for running custom web tests

## Pre-reqs
- Have a chromium enlistment with build working correctly, ensure git worktree is clean
- Install NodeJS 18.16.0

## 1. Clone repo
```shell
git clone https://github.com/rohanraja/xtrace-core
```
## 2. Install NodeJS
- Any latest version would work, last known working version is 18.16.0

## 3. Open xtrace-core folder in VSCode
- VScode will be the UI to interact with xtrace config

## 4. Run task - "Install: use"
- Hit "Ctrl+Shift+P" and search for "Run task" command, execute it.
- A list of tasks available are shown, search for the task to run "Install: use" and hit Enter

## 5. Run task - "Add new run config"
- Name the config according to your scenario
- The config json opens up, edit the file according to your case, check [this guide](#runconfig-json-file) on run_config.json.
- To test, the base file has config to run async clipboard scenarios

## 6. Run task - "Run E2E automation"
- This should trigger the complete e2e pipeline to hook code in chromium, compile, run chromium and then upload recording to RampEasy server
- Wait for few minutes for the pipeline to complete, should take 5-20 mins
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
- web_test: Relative path of the webtest that should be run to record code

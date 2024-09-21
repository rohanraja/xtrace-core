## About this automation script
This script automates the E2E pipeline for
1. Onboarding chromium repo to xTrace by:
    a.) Resetting current changes in repo (git reset --HARD)
    b.) Copying latest xtrace.h to cr third_party
    c.) Running hook injection code on selected folders
    d.) Uploading generated source code run json file to xTrace server

2. Compiling Chromium blink_tests and content_shell

3. Running WPT tests for xTrace scenarios of clipboard
    a.) Starts WPT server required for running WPT
    b.) Copies xTrace's WPT HTML file to chromium's folder
    c.) Running xTrace's WPT HTML
4. Uploads generated xtrace.run file so that the recording is available on the server.

## To run

1. Edit scripts/input.js with appropriate values
2. Ensure list of folders to inject is updated in hooks_injector/cpp_hooks_injector/inject_in_folders.ts

``` shell
cd <Root>/scripts/
npm install

cd <Root>
node scripts/run.js
```

### To partially run one or more steps
Mention step number as argument, comma separated

``` shell
cd <repo root> # e.g. cd C:\src\xtrace-core
node scripts/run.js 2,3 # Runs steps 2 and 3
node scripts/run.js 8 # Runs steps 8
```

## Logging

For each run, a log file is generated based on current datetime within /logs folder.
This can be used to investigate any compile errors or issues with executing the pipeline
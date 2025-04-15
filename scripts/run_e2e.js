/*
Input:
- Chromium source code "src" folder path (excluding src) 
- Chromium build directory (e.g. debug_full_x64)
- Ensure list of folders to inject is updated in 
  hooks_injector/cpp_hooks_injector/inject_in_folders.ts

*/

const path = require('path');
const fs = require('fs');
const uploadFile = require('../common/utils/web_utils.js').uploadFile;
const runStepWithFilter = require('./utils.js').runStep;
const { ChromeBuildScripts } = require('../clients/chromium/utils/chrome_build_scripts.js');
const JSON5 = require('json5');
const { convertFileToJsonArray } = require('./json_utils.js');
const { killAllProcessWithName, run } = require('../common/utils/process_utils.js');
const { chromeProcessImageName, contentShellProcessImageName } = require('../clients/chromium/utils/path_utils.js');

async function main() {

  let json_config = "";

  if(process.argv.length > 2){
      json_config = fs.readFileSync(process.argv[2], 'utf-8');
  }else{
      // Read sourcecode from stdin stream
      json_config = fs.readFileSync(0, 'utf-8');
  }

  const config = JSON5.parse(json_config);

  console.log("### Running with config: ", JSON.stringify(config, null, 2));

  let run_filter = config.run_filter;

  // If env has XT_RUN_FILTER, override the run_filter
  if(process.env["XT_RUN_FILTER"]){
    run_filter = process.env["XT_RUN_FILTER"];
    console.log("Overriding run_filter with: " + run_filter);
  }

  const runStep = (step, fn) => runStepWithFilter(step, fn, run_filter);

  const username = process.env["USERNAME"] || "unknown";
  const sno = process.env["XTRACE_SNO"] || "0";
  const code_run_name_prefix = `${username}/${config.name}/${sno}`;

  const test_input = config;

  // ChromeBuildConfig
  const chromeBuildConfig = {
    ...config,
    name: config.name,
    build_arch: config.build_arch,
    build_type: config.build_type,
    cr_path: config.cr_path,
    ut_target: config.ut_target,
  };

  const chromeBuildScript = new ChromeBuildScripts(chromeBuildConfig);

  // 0.1 Setup paths
  const isWin = process.platform === "win32";
  const cr_src_folder = path.join(test_input.cr_path, "src");

  const buildFolderName = `${test_input.build_type}_${test_input.build_arch}`;

  const cr_debug_folder = path.join(cr_src_folder, "out", test_input.debug_folder_name || buildFolderName);
  const cr_hooks_injector_folder = path.join(__dirname, '..', "hooks_injector", "cpp_hooks_injector");
  const upload_url = `http://${test_input.xtrace_server_ip}:${(test_input.xtrace_server_port || 3004)}/api/upload`;
  const xtrace_run_json = path.join(cr_debug_folder, 'xtrace.run.json');
  const xtrace_run_log = path.join(cr_debug_folder, 'xtrace.run.log');
  let content_shell_bin = isWin ? 'content_shell.exe' : '"./Content\ Shell.app/Contents/MacOS/Content\ Shell"';
  let chromium_bin = isWin ? 'chrome.exe' : './Chromium.app/Contents/MacOS/Chromium';

  // If linux, use "./chrome"
  const isLinux = process.platform == "linux";
  if(isLinux)
  {
    chromium_bin = "./chrome";
    content_shell_bin = "./content_shell";
  }


  let run_chrome = test_input.web_page != null && test_input.web_page != "" && test_input.web_page != undefined;

  let web_page = test_input.web_page;
  if(run_chrome){
    // If web_page doesn't start with http, construct file:// url from relative path in pwd
    if(!web_page.startsWith("http")){
      web_page = `file://${path.join(process.cwd(), "run_configs/html", web_page)}`;
    }

  }else{
    if(run_filter.includes("-chrome")){
      web_page = "http://google.com";
      run_chrome = true;
    }
  }


  // 0.2 Setup environment to include cr tools like autoninja
  let envs = process.env;
  if (isWin) {
    envs = { ...process.env, Path: `C:\\Program Files\\nodejs;${test_input.cr_path}\\depot_tools\\scripts;${test_input.cr_path}\\depot_tools;${process.env.Path}` };
  } else {
    // For Linux and macOS, PATH is uppercase
    envs = { ...process.env, PATH: `${test_input.cr_path}/chromium.depot_tools.cr-contrib/scripts:${test_input.cr_path}/chromium.depot_tools.cr-contrib:${process.env.PATH}` };
  }
  envs = {...envs, "XTRACE_CONFIG": JSON.stringify(config)}
  envs = {...envs, "XTRACE_PREFIX": code_run_name_prefix}

  const runInEnv = (command, cwd) => run(command, cwd, envs);
  const runInEnvWaited = (command, cwd) => run(command, cwd, {...envs, "WAIT_FOR_EXIT": "true"});


  // 1. Reset hard to HEAD for complete chromium repo
  await runStep("git-reset", async () => {
    await runInEnv(`git reset --hard`, cr_src_folder);
  });

  let branchChanged = false;
  let base_pulled = false;
  // Check if CL needs to be pulled
  await runStep("cl-fetch", async () => {
    const cl = test_input.cl;
    const patchSet = test_input.patch_set;
    const branch = test_input.branch;
    // If no cl or patch set, just return
    if(!cl && !patchSet && !branch){
      return;
    }

    const currentBranch = await runInEnv(`git rev-parse --abbrev-ref HEAD`, cr_src_folder);
    if(cl && patchSet){ 

      const targetBranch = `change-${cl}-${patchSet}`;
      console.log("Current branch: ", currentBranch);
      if(currentBranch.includes(targetBranch)){
        console.log("Already on branch: ", targetBranch);
      }
      else{
        console.log("Fetching and checking out branch: ", targetBranch);
        const patchSetLastTwoDigits = cl.slice(-2);
        const fetchUrl = `refs/changes/${patchSetLastTwoDigits}/${cl}/${patchSet}`;
        await runInEnv(`git checkout -b ${targetBranch} origin/main`, cr_src_folder);
        await runInEnv(`git fetch https://chromium.googlesource.com/chromium/src ${fetchUrl}`, cr_src_folder);
        // Merge FETCH_HEAD into current branch
        await runInEnv(`git merge FETCH_HEAD`, cr_src_folder);
        branchChanged = true;
      }

    } else if(branch){
      if(currentBranch == branch){
        return;
      }

      console.log("Fetching and checking out branch: ", branch);
      await runInEnv(`git fetch origin main`, cr_src_folder);
      await runInEnv(`git checkout -b ${branch} origin/main`, cr_src_folder);
      branchChanged = true;
    }
  });

  await runStep("gclient", async () => {
    if(base_pulled || test_input.regenerateBuildFolder || run_filter.includes("gclient")){
      console.log("Running gclient sync -fD");
      await runInEnv(`gclient sync -fD`, cr_src_folder);
    }else{
      console.log("Branch not changed, skipping gclient sync");
    }
  });

  await runStep("autogn-configure", async () => {
    if(test_input.regenerateBuildFolder){
      console.log("Running autogn");
      if(fs.existsSync(cr_debug_folder)){
        // Delete the build folder if it exists
        console.log(`Deleting build folder: ${cr_debug_folder}`);
        fs.rmSync(cr_debug_folder, { recursive: true, force: true });
      }
      await runInEnv(`autogn ${test_input.build_arch} ${test_input.build_type}`, cr_src_folder);
    }else{
      console.log("Regenerate build folder not set, skipping autogn");
    }
  });

  // 2. Copy xTrace recorder folder from xtrace-core to
  await runStep("copy-xtrace-lib", async () => {
    const srcFolder = path.join(__dirname, '..', "cpp_recorder", "xtrace");
    const destFolder = path.join(cr_src_folder, 'third_party', 'xtrace');
    await fs.promises.cp(srcFolder, destFolder, { recursive: true });
    console.log(`Copied ${srcFolder} to ${destFolder}`);

    // 2.3 Delete xTrace.run.json if it exists
    if (fs.existsSync(xtrace_run_json)) {
      fs.rmSync(xtrace_run_json);
    }

    // 2.4 Delete content_shell.exe if it exists, so no tests run if build failed
    const content_shell_path = path.join(cr_debug_folder, content_shell_bin);
    if (fs.existsSync(content_shell_path)) {
      fs.rmSync(content_shell_path);
    }
  });

  // 3. Run hook injection on selected folders as per input
  await runStep("inject-hooks", async () => {
    await runInEnv(`npm run folders`, cr_hooks_injector_folder);
  });

  // 4. Upload source code run events to xTrace server
  await runStep("upload-source-code", async () => {
    const xtrace_sourcecode_events_path = path.join(cr_hooks_injector_folder, 'code_events.json');
    await uploadFile(xtrace_sourcecode_events_path, upload_url);
  });

  // 5. Build chromium code
  // TODO - Check if build failed then exit
  // await runStep("build-webtest", async () => {
  //   if(!run_chrome){
  //     return chromeBuildScript.buildContentShell();
  //   }
  // });

  await runStep("build-chrome", async () => {
    return chromeBuildScript.buildChrome();
    if(run_chrome){
      console.log("Building chrome");

      return chromeBuildScript.buildChrome();

      // Close any running process
      try{
        killAllProcessWithName(chromeProcessImageName());
      }catch(e){
        console.log("No chrome running");
      }
      console.log("Building chrome");
      await runInEnv(`autoninja chrome`, cr_debug_folder);
    }
    // await runInEnv(`autoninja blink_tests`, cr_debug_folder);
  });

  if(!run_chrome && !test_input.should_skip_wpt_serve){
    await runStep("wpt-serve", async () => {
      runInEnv(`vpython3 third_party/blink/tools/run_blink_wptserve.py -t ${test_input.debug_folder_name}`, cr_src_folder);

      // Wait for 5 seconds
      await new Promise(resolve => setTimeout(resolve, 10000));
    });
  }

  let alreadyRan = false;
  await runStep("run-tests", async () => {

    if(test_input.ut_filter) {
      console.log("## Running unit tests with filter: ", test_input.ut_filter);
      if (fs.existsSync(xtrace_run_json)) {
        fs.rmSync(xtrace_run_json);
      }
      if (fs.existsSync(xtrace_run_log)) {
        fs.rmSync(xtrace_run_log);
      }
    // await runInEnv(`${content_shell_bin}  --run-web-tests --no-sandbox http://localhost:8001/clipboard-apis/async-navigator-clipboard-xtrace.html`, cr_debug_folder);
    try{
      await runInEnv(`./${test_input.ut_target} --no-sandbox --gtest_filter='*${test_input.ut_filter}*'`, cr_debug_folder);
    }catch(e){
      console.log("Error running unit tests, skipping web test");
    }finally{
      // Delay for 5 seconds for xtrace.run.json to be generated
      console.log("Delaying for 5 seconds for xtrace.run.json to be generated");
      await new Promise(resolve => setTimeout(resolve, 5000));
      alreadyRan = true;
    }

    }
  });

  await runStep("run-web-test", async () => {

    if(alreadyRan){
      console.log("## Already ran unit tests, skipping web test");
      return;
    }
    if (fs.existsSync(xtrace_run_json)) {
      fs.rmSync(xtrace_run_json);
    }
    if (fs.existsSync(xtrace_run_log)) {
      fs.rmSync(xtrace_run_log);
    }
    // await runInEnv(`${content_shell_bin}  --run-web-tests --no-sandbox http://localhost:8001/clipboard-apis/async-navigator-clipboard-xtrace.html`, cr_debug_folder);
    if(!run_chrome){
      await runInEnv(`${content_shell_bin}  --run-web-tests --no-sandbox ${test_input.web_test}`, cr_debug_folder);

      // Delay for 5 seconds for xtrace.run.json to be generated
      await setTimeout(() => {}, 5000);
    }
  });

  await runStep("run-chrome", async () => {
    if(alreadyRan){
      console.log("## Already ran unit tests, skipping web test");
      return;
    }

    if (fs.existsSync(xtrace_run_json)) {
      fs.rmSync(xtrace_run_json);
    }
    if (fs.existsSync(xtrace_run_log)) {
      fs.rmSync(xtrace_run_log);
    }
    // await runInEnv(`${content_shell_bin}  --run-web-tests --no-sandbox http://localhost:8001/clipboard-apis/async-navigator-clipboard-xtrace.html`, cr_debug_folder);
    if(run_chrome){
      await runInEnvWaited(`${chromium_bin}  --no-sandbox ${web_page}`, cr_debug_folder);
    }
  });

  // 8. Upload scenario recording xtrace.run file to xTrace server
  await runStep("upload-recording", async () => {
    console.log("Converting logs to json");
    await convertFileToJsonArray(xtrace_run_log, xtrace_run_json);
    console.log("Uploading xtrace.run.json");
    await uploadFile(xtrace_run_json, upload_url);
    const recording_url = `http://${test_input.xtrace_server_ip}:${test_input.xtrace_server_port ? test_input.xtrace_server_port + 5 : 3009}/?user=${encodeURIComponent(code_run_name_prefix)}`;
    console.log(`Visit ${recording_url} to view the trace`);
    // Write url to tmp/last_recording_url
    fs.writeFileSync('tmp/last_recording_url', recording_url);
  });

  // Kill process
  process.exit(0);

}

main();



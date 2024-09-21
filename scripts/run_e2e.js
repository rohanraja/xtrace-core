
/*
Input:
- Chromium source code "src" folder path (excluding src) 
- Chromium build directory (e.g. debug_full_x64)
- Ensure list of folders to inject is updated in 
  hooks_injector/cpp_hooks_injector/inject_in_folders.ts

*/

const path = require('path');
const fs = require('fs');
const run = require('./utils.js').run;
const uploadFile = require('./utils.js').uploadFile;
const test_input = require('./input.js').test_input;
const runStep = require('./utils.js').runStep;

async function main() {

  // 0.1 Setup paths
  const cr_src_folder = path.join(test_input.cr_path, "src");
  const cr_debug_folder = path.join(cr_src_folder, "out", test_input.debug_folder_name);
  const cr_hooks_injector_folder = path.join(__dirname, '..', "hooks_injector", "cpp_hooks_injector");
  const upload_url = `http://${test_input.xtrace_server_ip}:3004/api/upload`;
  const xtrace_run_json = path.join(cr_debug_folder, 'xtrace.run.json');

  // 0.2 Setup environment to include cr tools like autoninja
  const isWin = process.platform === "win32";
  let envs = process.env;
  if (isWin) {
    envs = { ...process.env, Path: `C:\\Program Files\\nodejs;${test_input.cr_path}\\depot_tools\\scripts;${test_input.cr_path}\\depot_tools;${process.env.Path}` };
  }

  const runInEnv = (command, cwd) => run(command, cwd, envs);

  // 1. Reset hard to HEAD for complete chromium repo
  await runStep("1", async () => {
    await runInEnv(`git reset --hard`, cr_src_folder);
  });

  // 2. Copy xTrace recorder folder from xtrace-core to
  await runStep("2", async () => {
    const srcFolder = path.join(__dirname, '..', "cpp_recorder", "xtrace");
    const destFolder = path.join(cr_src_folder, 'third_party', 'xtrace');
    await fs.promises.cp(srcFolder, destFolder, { recursive: true });
    console.log(`Copied ${srcFolder} to ${destFolder}`);

    // 2.2 Copy xTrace WPT folder from xtrace-core to chromium WPTs
    const xt_test_path = path.join(__dirname, '..', "test_data", "async-navigator-clipboard-xtrace.html");
    const wpt_xt_test_path = path.join(cr_src_folder, 'third_party', 'blink', 'web_tests', 'external', 'wpt', 'clipboard-apis', "async-navigator-clipboard-xtrace.html");
    await fs.copyFileSync(xt_test_path, wpt_xt_test_path);
    console.log(`Copied ${xt_test_path} to ${wpt_xt_test_path}`);

    // 2.3 Delete xTrace.run.json if it exists
    if (fs.existsSync(xtrace_run_json)) {
      fs.rmSync(xtrace_run_json);
    }
  });

  // 3. Run hook injection on selected folders as per input
  await runStep("3", async () => {
    await runInEnv(`npm run folders`, cr_hooks_injector_folder);
  });

  // 4. Upload source code run events to xTrace server
  await runStep("4", async () => {
    const xtrace_sourcecode_events_path = path.join(cr_hooks_injector_folder, 'code_events.json');
    await uploadFile(xtrace_sourcecode_events_path, upload_url);
  });

  // 5. Build chromium code
  await runStep("5", async () => {
    console.log("Building content_shell");
    await runInEnv(`autoninja content_shell`, cr_debug_folder);

    console.log("Building blink_tests");
    await runInEnv(`autoninja blink_tests`, cr_debug_folder);
  });

  // 6. Run chrome
  // await runStep("6", async () => {
  //   const binary_name = isWin ? 'chrome.exe' : 'chrome';
  //   await runInEnv(`${binary_name} --no-sandbox`, cr_debug_folder);
  // });

  await runStep("6.8", async () => {
    runInEnv(`vpython3 third_party/blink/tools/run_blink_wptserve.py -t ${test_input.debug_folder_name}`, cr_src_folder);

    // Wait for 5 seconds
    await new Promise(resolve => setTimeout(resolve, 10000));
  });

  await runStep("7", async () => {
    await runInEnv(`content_shell.exe  --run-web-tests --no-sandbox http://localhost:8001/clipboard-apis/async-navigator-clipboard-xtrace.html`, cr_debug_folder);
  });

  // 8. Upload scenario recording xtrace.run file to xTrace server
  await runStep("8", async () => {
    await uploadFile(xtrace_run_json, upload_url);
  });

  // Kill process
  process.exit(0);

}

main();



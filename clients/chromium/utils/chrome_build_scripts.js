const path = require('path');
const fs = require('fs');
const { AppBuildScriptBase } = require('../../../common/utils/app_build_script_base.js');
const { killAllProcessWithName } = require('../../../common/utils/process_utils.js');
const { 
  chromeProcessImageName, 
  contentShellProcessImageName,
  getChromiumBuildBinPath,
  getContentShellBuildBinPath 
} = require('./path_utils.js');

/**
 * @typedef {Object} ChromeBuildConfig
 * @property {string} name - Build name
 * @property {string} build_arch - Build architecture (e.g. 'x64')
 * @property {string} build_type - Build type (e.g. 'debug')
 * @property {string} cr_path - Path to Chromium root folder
 * @property {string} [debug_folder_name] - Custom name for the build output folder
 * @property {boolean} [regenerateBuildFolder] - Whether to regenerate build folder
 * @property {string} [run_filter] - Filter for which steps to run
 * @property {string} [branch] - Git branch to checkout
 * @property {string} [cl] - Chromium CL number
 * @property {string} [patch_set] - Patch set number
 * @property {string} [web_test] - Path to web test to run
 * @property {string} [web_page] - Web page URL to run in Chrome
 * @property {boolean} [should_skip_wpt_serve] - Skip starting the WPT server
 * @property {string} [xtrace_server_ip] - xTrace server IP address
 */

/**
 * Helper class to handle Chromium build operations
 */
class ChromeBuildScripts extends AppBuildScriptBase {
  /**
   * Creates a new instance of ChromeBuildScripts
   * @param {ChromeBuildConfig} config - Configuration for Chrome build
   */
  constructor(config) {
    // Setup environment
    const env = {...process.env};
    if (process.platform === "win32") {
      env.Path = `C:\\Program Files\\nodejs;${config.cr_path}\\depot_tools\\scripts;${config.cr_path}\\depot_tools;${process.env.Path}`;
    } else {
      // For Linux and macOS, PATH is uppercase
      env.PATH = `${config.cr_path}/chromium.depot_tools.cr-contrib/scripts:${config.cr_path}/chromium.depot_tools.cr-contrib:${process.env.PATH}`;
    }
    
    // Call parent constructor with environment
    super(env);

    this.config = config;
    this.src_path = path.join(config.cr_path, "src");
    
    // Generate the build folder name
    this.buildFolderName = `${config.build_type}_${config.build_arch}`;
    this.build_path = path.join(this.src_path, "out", config.debug_folder_name || this.buildFolderName);

    // Setup binary names based on platform using path_utils
    this.content_shell_bin = getContentShellBuildBinPath();
    this.chromium_bin = getChromiumBuildBinPath();
    
    
    this.branchChanged = false;
  }
  
  /**
   * Reset the git repository to HEAD
   * @returns {Promise<void>}
   */
  async gitReset() {
    console.log("Resetting git repository to HEAD");
    await this.runCommand("git reset --hard", this.src_path);
  }

  /**
   * Fetch and checkout a specific branch, CL or patch set
   * @returns {Promise<void>}
   */
  async clFetch() {
    const { cl, patch_set, branch } = this.config;
    
    // If no cl or patch set, just return
    if (!cl && !patch_set && !branch) {
      return;
    }

    const currentBranch = await this.runCommand("git rev-parse --abbrev-ref HEAD", this.src_path);
    
    if (cl && patch_set) {
      const targetBranch = `change-${cl}-${patch_set}`;
      console.log("Current branch: ", currentBranch);
      
      if (currentBranch.includes(targetBranch)) {
        console.log("Already on branch: ", targetBranch);
      } else {
        console.log("Fetching and checking out branch: ", targetBranch);
        const patchSetLastTwoDigits = cl.slice(-2);
        const fetchUrl = `refs/changes/${patchSetLastTwoDigits}/${cl}/${patch_set}`;
        await this.runCommand(
          `git fetch https://chromium.googlesource.com/chromium/src ${fetchUrl}`,
          this.src_path
        );
        await this.runCommand(`git checkout -b ${targetBranch} FETCH_HEAD`, this.src_path);
        this.branchChanged = true;
      }
    } else if (branch) {
      if (currentBranch === branch) {
        return;
      }

      console.log("Fetching and checking out branch: ", branch);
      await this.runCommand("git fetch origin main", this.src_path);
      await this.runCommand(`git checkout -b ${branch} origin/main`, this.src_path);
      this.branchChanged = true;
    }
  }

  /**
   * Run gclient sync if branch has changed
   * @returns {Promise<void>}
   */
  async gclientSync() {
    if (this.branchChanged) {
      console.log("Running gclient sync -fD");
      await this.runCommand("gclient sync -fD", this.src_path);
    } else {
      console.log("Branch not changed, skipping gclient sync");
    }
  }

  /**
   * Configure autogn for the build
   * @returns {Promise<void>}
   */
  async autognConfigure() {
    if (this.config.regenerateBuildFolder) {
      console.log("Running autogn");
      if (fs.existsSync(this.build_path)) {
        // Delete the build folder if it exists
        console.log(`Deleting build folder: ${this.build_path}`);
        fs.rmSync(this.build_path, { recursive: true, force: true });
      }
      await this.runCommand(
        `autogn ${this.config.build_arch} ${this.config.build_type}`,
        this.src_path
      );
    } else {
      console.log("Regenerate build folder not set, skipping autogn");
    }
  }

  /**
   * Copy external libraries to the Chromium source tree
   * @param {string} srcFolder - Source folder path
   * @param {string} destName - Destination folder name
   * @returns {Promise<void>}
   */
  async copyExternalLibrary(srcFolder, destName) {
    const destFolder = path.join(this.src_path, 'third_party', destName);
    await fs.promises.cp(srcFolder, destFolder, { recursive: true });
    console.log(`Copied ${srcFolder} to ${destFolder}`);
  }

  /**
   * Build Chrome browser
   * @returns {Promise<void>}
   */
  async buildChrome() {
    console.log("Building Chrome browser");
    try {
      await killAllProcessWithName(chromeProcessImageName());
    } catch(e) {
      console.log("No Chrome process running or couldn't kill it");
    }
    
    await this.runCommand("autoninja chrome", this.build_path);
  }

  /**
   * Build content_shell
   * @returns {Promise<void>}
   */
  async buildContentShell() {
    console.log("Building content_shell");
    try {
      await killAllProcessWithName(contentShellProcessImageName());
    } catch(e) {
      console.log("No content_shell running");
    }
    
    await this.runCommand("autoninja content_shell", this.build_path);
  }

  /**
   * Start the WPT server
   * @returns {Promise<void>}
   */
  async startWPTServer() {
    console.log("Starting WPT server");
    await this.runCommand(
      `vpython3 third_party/blink/tools/run_blink_wptserve.py -t ${this.config.debug_folder_name || this.buildFolderName}`,
      this.src_path
    );
    
    // Wait for server to start
    await new Promise(resolve => setTimeout(resolve, 10000));
  }

  /**
   * Run web tests with content_shell
   * @param {string} [testPath] - Path to the test
   * @returns {Promise<void>}
   */
  async runWebTest(testPath) {
    if (!testPath && this.config.web_test) {
      testPath = this.config.web_test;
    }
    
    if (!testPath) {
      throw new Error("No web test path provided");
    }
    
    console.log(`Running web test: ${testPath}`);
    await this.runCommand(
      `${this.content_shell_bin} --run-web-tests --no-sandbox ${testPath}`,
      this.build_path
    );
    
    // Wait for test to complete
    await new Promise(resolve => setTimeout(resolve, 5000));
  }

  /**
   * Run Chrome with a specific web page
   * @param {string} [webPage] - URL to load
   * @returns {Promise<void>}
   */
  async runChrome(webPage) {
    let pageUrl = webPage || this.config.web_page;
    
    if (!pageUrl) {
      pageUrl = "http://google.com";
    }
    
    // If web_page doesn't start with http, construct file:// url
    if (!pageUrl.startsWith("http")) {
      pageUrl = `file://${path.join(process.cwd(), "run_configs/html", pageUrl)}`;
    }
    
    console.log(`Running Chrome with page: ${pageUrl}`);
    await this.runCommand(
      `${this.chromium_bin} --no-sandbox ${pageUrl}`,
      this.build_path,
      true // Wait for exit
    );
  }

  /**
   * Run a complete build pipeline
   * @param {Object} [options] - Build options
   * @param {boolean} [options.skipGitReset] - Skip the git reset step
   * @param {boolean} [options.skipClFetch] - Skip the cl fetch step
   * @param {boolean} [options.buildChrome] - Build Chrome
   * @param {boolean} [options.buildContentShell] - Build content_shell
   * @returns {Promise<void>}
   */
  async runFullBuild(options = {}) {
    // Git operations
    if (!options.skipGitReset) {
      await this.gitReset();
    }
    
    if (!options.skipClFetch) {
      await this.clFetch();
      await this.gclientSync();
    }
    
    // Build setup
    await this.autognConfigure();
    
    // Build targets
    if (options.buildContentShell !== false) {
      await this.buildContentShell();
    }
    
    if (options.buildChrome === true) {
      await this.buildChrome();
    }
  }
}

module.exports = { ChromeBuildScripts };
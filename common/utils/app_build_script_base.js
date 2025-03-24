const { exec } = require('child_process');
const fs = require('fs');
const path = require('path');

/**
 * Base class for application build scripts with common functionality
 */
class AppBuildScriptBase {
  /**
   * Creates a new instance of AppBuildScriptBase
   * @param {Object} env - Environment variables
   */
  constructor(env = process.env) {
    this.env = {...env};
  }
  
  /**
   * Run a command in the specified directory with the configured environment
   * @param {string} command - Command to run
   * @param {string} cwd - Working directory
   * @param {boolean} [waitForExit=false] - Whether to wait for the command to exit
   * @returns {Promise<string>} Command output
   */
  async runCommand(command, cwd, waitForExit = false) {
    const env = waitForExit ? {...this.env, "WAIT_FOR_EXIT": "true"} : this.env;
    return new Promise((resolve, reject) => {
      console.log(`Running command: ${command} in ${cwd}`);
      exec(command, { cwd, env }, (error, stdout, stderr) => {
        if (error) {
          console.error(`Error running command: ${error.message}`);
          console.error(`stderr: ${stderr}`);
          reject(error);
          return;
        }
        
        if (stderr) {
          console.warn(`stderr: ${stderr}`);
        }
        
        resolve(stdout.trim());
      });
    });
  }

  /**
   * Generates a VS Code tasks.json configuration for all methods in this class
   * @param {Object} options - Configuration options
   * @param {string} [options.scriptPath] - Path to the main script file that uses this class
   * @param {string} [options.outputPath] - Path where tasks.json should be written
   * @param {string} [options.className] - Name of the class to show in task labels
   * @param {string[]} [options.excludeMethods] - Methods to exclude from task generation
   * @returns {Object} The tasks.json configuration object
   */
  generateVSCodeTasksJSON(options = {}) {
    // Default options
    const {
      scriptPath = 'script.js',
      outputPath = '.vscode/tasks.json',
      className = this.constructor.name,
      excludeMethods = ['constructor', 'generateVSCodeTasksJSON', 'runCommand']
    } = options;
    
    // Get all method names from the class prototype
    const methods = Object.getOwnPropertyNames(Object.getPrototypeOf(this))
      .filter(name => {
        // Filter out excluded methods and anything starting with underscore (private)
        return typeof this[name] === 'function' && 
               !excludeMethods.includes(name) && 
               !name.startsWith('_');
      });

    // Create tasks for each method
    const tasks = methods.map(methodName => {
      return {
        label: `${className}: ${methodName}`,
        type: 'shell',
        command: `node ${scriptPath} --method ${methodName}`,
        group: {
          kind: 'build',
          isDefault: methodName === 'runFullBuild'
        },
        presentation: {
          reveal: 'always',
          panel: 'new',
          clear: true
        },
        problemMatcher: []
      };
    });

    // Create the full tasks.json object
    const tasksConfig = {
      version: '2.0.0',
      tasks
    };

    // Create the output directory if it doesn't exist
    if (outputPath) {
      const outputDir = path.dirname(outputPath);
      if (!fs.existsSync(outputDir)) {
        fs.mkdirSync(outputDir, { recursive: true });
      }
      
      // Write the tasks.json file
      fs.writeFileSync(
        outputPath, 
        JSON.stringify(tasksConfig, null, 2),
        'utf8'
      );
      
      console.log(`VS Code tasks.json written to: ${outputPath}`);
    }

    return tasksConfig;
  }
}

module.exports = { AppBuildScriptBase };
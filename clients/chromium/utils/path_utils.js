
function chromeBinaryPath(){
    switch (process.platform) {
        case 'win32':
            return "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe";
        case 'darwin':
            return "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
        default:
            return "/usr/bin/google-chrome";
    }
}

function chromeProcessImageName(){
    switch (process.platform) {
        case 'win32':
            return "chrome.exe";
        case 'darwin':
            return "chrome";
        default:
            return "chrome";
    }
}

function contentShellProcessImageName(){
    switch (process.platform) {
        case 'win32':
            return "content_shell.exe";
        case 'darwin':
            return "Content Shell";
        default:
            return "Content Shell";
    }
}

/**
 * Get the relative path to the Chromium binary in a build directory
 * @returns {string} Relative path to Chromium binary
 */
function getChromiumBuildBinPath() {
    switch (process.platform) {
        case 'win32':
            return "chrome.exe";
        case 'darwin':
            return './Chromium.app/Contents/MacOS/Chromium';
        default:
            return "./chrome";
    }
}

/**
 * Get the relative path to the Content Shell binary in a build directory
 * @returns {string} Relative path to Content Shell binary
 */
function getContentShellBuildBinPath() {
    switch (process.platform) {
        case 'win32':
            return "content_shell.exe";
        case 'darwin':
            return '"./Content\\ Shell.app/Contents/MacOS/Content\\ Shell"';
        default:
            return "./content_shell";
    }
}

module.exports = {
    chromeBinaryPath,
    chromeProcessImageName,
    contentShellProcessImageName,
    getChromiumBuildBinPath,
    getContentShellBuildBinPath
};
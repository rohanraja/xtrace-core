
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
            return "Chromium";
        default:
            return "Chromium";
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

module.exports = {
    chromeBinaryPath,
    chromeProcessImageName,
    contentShellProcessImageName
};
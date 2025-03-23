const { exec } = require('child_process');

var start = (process.platform == 'darwin'? 'open': process.platform == 'win32'? 'start': 'xdg-open');

const openUrl = (url) => {
    exec(`${start} ${url}`);
};

module.exports = {
    openUrl
};
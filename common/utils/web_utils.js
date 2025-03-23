const { exec } = require('child_process');
const FormData = require('form-data');
const fs = require('fs');
const axios = require('axios');

var start = (process.platform == 'darwin'? 'open': process.platform == 'win32'? 'start': 'xdg-open');

const openUrl = (url) => {
    exec(`${start} ${url}`);
};


async function uploadFile(filePath, url) {
    const form = new FormData();
    form.append('file', fs.createReadStream(filePath));
  
    try {
      const response = await axios.post(url, form, {
        headers: {
          ...form.getHeaders(),
        },
      });
      console.log('File uploaded successfully:', response.data);
    } catch (error) {
      console.error('Error uploading file:', error);
    }
}

module.exports = {
    openUrl,
    uploadFile
};
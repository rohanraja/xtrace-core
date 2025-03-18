
function cleanTextForSnapshot(text) {
    // For text like [24-byte object at 0x0x16db0de10], replace with [object]
    const regex = /0x[0-9a-fA-F]+\]/g;
    const cleanedText = text.replace(regex, '[object]');
    // Remove the "object" part and the address
    return cleanedText;

}

module.exports = {
    cleanTextForSnapshot
};
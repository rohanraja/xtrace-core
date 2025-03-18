const { cleanTextForSnapshot } = require("../utils/snapshot_utils");

describe('Snpashot utils test', () => {

    test("simple snapshot test", () => {
        const str = '["ID_2","SEND_VAR_UPDATE","[\"ID_3\",\"numbers\",\"VALUE\",\"\",\"[24-byte object at 0x0x16db0de10]\",\"4\",\"true\"]"]';
        const op = cleanTextForSnapshot(str);
        expect(op).toContain("[24-byte object at 0x[object]");
    });

});
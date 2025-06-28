import * as fs from 'fs';
import * as path from 'path';

export interface InjectConfig {
    cr_path: string; // EXCLUDING "src" folder
    debug_folder_name: string;
    xtrace_server_ip: string;
    path_prefix_filter: string;
    folders_whitelist: string[];
    files_whitelist: string[];
    methods_whitelist: string[];
    methods_blacklist: string[];
    ignored_types: string[];
    methods_which_split_run: string[];
    name: string;
};


export function GetConfigFromEnv() {
    let config: InjectConfig;
    if (process.env["XTRACE_CONFIG"]) {
        config = JSON.parse(process.env["XTRACE_CONFIG"]);
    } else {
        // Read config from filepath provided as argument
        const jsonFilePath = path.join('default_config.json');
        const jsonData = fs.readFileSync(jsonFilePath, 'utf8');
        config = JSON.parse(jsonData);
    }
    return config;
}
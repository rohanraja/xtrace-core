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
    /**
     * Skip variable hooking (LocalVarUpdate calls) while preserving method entry and line logging.
     * When true, only OnMethodEnter and LogLineRun calls will be generated.
     * Default: false (maintains backward compatibility)
     */
    skipVariablesHooking?: boolean;
};


export function GetConfigFromEnv() {
    // Always start with default config
    const jsonFilePath = path.join('default_config.json');
    const jsonData = fs.readFileSync(jsonFilePath, 'utf8');
    let config: InjectConfig = JSON.parse(jsonData);
    
    // If environment override is provided, merge it with default config
    if (process.env["XTRACE_CONFIG"]) {
        const envConfig = JSON.parse(process.env["XTRACE_CONFIG"]);
        config = { ...config, ...envConfig };
    }
    
    return config;
}
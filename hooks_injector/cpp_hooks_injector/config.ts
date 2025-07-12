import { GetConfigFromEnv } from './inject_config';

export const config = GetConfigFromEnv();
export let fileName = process.env["FileName"] || "main.cc";
export let cvid = process.env["CodeVersion"] || "3c4e3b6b-2026-4b15-872c-07ce4463f59b";
export const methodsToInclude = config.methods_whitelist || [];
export const methodsToExclude = config.methods_blacklist || [];
export const typesToExclude = config.ignored_types || [];
export const primitiveTypes = ["int", "float", "double", "char", "string", "bool"];
export const skipVariablesHooking = config.skipVariablesHooking || false;

// Temporary override for skipVariablesHooking (used by HTTP API)
let tempSkipVariablesHooking: boolean | undefined = undefined;

/**
 * Helper function to check if variable hooking should be skipped
 * @returns {boolean} true if variable hooking should be skipped, false otherwise
 */
export function shouldSkipVariablesHooking(): boolean {
    // Use temporary override if set, otherwise use config value
    return tempSkipVariablesHooking !== undefined ? tempSkipVariablesHooking : skipVariablesHooking;
}

/**
 * Temporarily override the skipVariablesHooking setting (used by HTTP API)
 * @param override - boolean value to override with, or undefined to clear override
 */
export function setSkipVariablesHookingOverride(override: boolean | undefined) {
    tempSkipVariablesHooking = override;
}

export function setFileAndVersion(file: string, version: string) {
    fileName = file;
    cvid = version;
    process.env["FileName"] = file;
    process.env["CodeVersion"] = version;
}
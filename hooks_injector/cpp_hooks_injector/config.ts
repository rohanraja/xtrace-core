import { GetConfigFromEnv } from './inject_config';

export const config = GetConfigFromEnv();
export const fileName = process.env["FileName"] || "main.cc";
export const cvid = process.env["CodeVersion"] || "3c4e3b6b-2026-4b15-872c-07ce4463f59b";
export const methodsToInclude = config.methods_whitelist || [];
export const methodsToExclude = config.methods_blacklist || [];
export const primitiveTypes = ["int", "float", "double", "char", "string", "bool"];
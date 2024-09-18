const esbuild = require("esbuild");

esbuild
  .build({
    entryPoints: ["inject_in_folders.ts"], // Your input file
    bundle: true, // Bundle all dependencies into one file
    platform: "node", // Specify the platform (node for Node.js)
    // target: "node18.16.0", // ECMAScript target version
    target: "ESNext", // ECMAScript target version
    outfile: "dist/inject_folders.js", // The output file for the bundled code
    external: ["*.node", "tree-sitter", "tree-sitter-cpp"], // Exclude these modules from the bundle
  })
  .catch(() => process.exit(1));

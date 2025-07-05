const esbuild = require("esbuild");

esbuild
  .build({
    entryPoints: ["serve.ts"], // Server entry point
    bundle: true,
    platform: "node",
    target: "ESNext",
    outfile: "dist/serve.js",
    external: ["*.node", "tree-sitter", "tree-sitter-cpp"], // Exclude these modules from the bundle
    sourcemap: true,
  })
  .catch(() => process.exit(1));

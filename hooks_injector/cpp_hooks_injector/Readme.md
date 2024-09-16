## 1. Clone repo
```shell
git clone https://github.com/rohanraja/xtrace-core
```

## 2. To install and test, run the following commands

```shell
cd hooks_injector/cpp_hooks_injector
npm install
npm run build
npm run test
```

After running the last command, code with xTrace logs should be generated in tests/out.cc.

## 3. To debug in VSCode

1. Put breakpoint in the generated JS code in "dist/out.js"
2. Run "Debug: Javascript Debug Terminal" from VSCode command.
3. Within the terminal run "npm run test"

## 4. To add a new feature, we can follow these steps:

1. Find a test cpp code snippet for your scenario. E.g. if working to add support for class, find a real cpp code which has the class definition.
2. Replace contents of "tests/inp.cc" with your test cpp snippet.
3. Run "npm run test" to generate "tests/out.cc" with current output.
4. Make code changes until expected output is generated.
5. If needed, debug as described in Step 3.)
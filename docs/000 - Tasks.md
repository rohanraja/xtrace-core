### Now
#now 

### P0
- Fix code run upload for 6175802
    - Check seq diag generator causing error 
- While building from CL, simply checkout local main (updated daily) then merge changes of the CL on top of it to
  speed up the build
  - Give an option of clean checkout vs merge on local latest
### Code parsing and injection logic
- Refactor to make index.ts code clean #now
- Fix Lambda function parsing #now 
- For CR files which worked, create unit tests
- Fix error where a To_string method can cause infinite recursion. #P0
    - e.g. in bool IsEditable(const Node &node) {
#### CR files to fix
- third_party/blink/renderer/core/editing/text_offset_mapping.cc
- third_party/blink/renderer/core/editing/markers/document_marker_controller.cc
#### CPP vars capturing scenarios
- Fix var state capture in for loops
- Capture state in const declaration
```cpp
const InlineContents inline_contents = ComputeInlineContentsFromNode(*node);
```
- Capture vars declared with "&"
    ```cpp
	const PositionInFlatTree& anchor_position =
        original_anchor_in_flat_tree_.GetPosition();
    ```

#### Error handling upon code hooking
- After hooking, check for errors by parsing again.
    - If error found, try to find the method name, and ignore it automatically
    - Log the error and report to me.
- For methods which are blacklisted and not important, keep a global map of this info
    - which can be reused by anyone


#### UT and tests
- Add more unit tests for cpp hooks injector to protect against refactor


### User experience

### Console UI
- Add logs selector
- Add user based login
- Restructure configs data with following schema
```
    - users
        - <UserID>
            - configs
                - <ConfigID>
                    - runs
                        - <RunID>
                            - logs.log
                            - finalUrl.txt
                            - runConfig.json5 
```
- Fix NextJS depedencies, create package.json on Node 20.0.0 or latest
- Show logs as markdown to highlight important stages with ###

### Distribution and adoption
- Create infra for xtrace-core where people can redistribute / use xtrace, recieve updates, etc
- Try user guide / infra on a new devbox machine
- Create a demo video showcasing how to use xtrace-core
- Create a single line script which automates cloning, installing deps, opening vscode

### E2E testing
- Create E2E test

### Inner dev loop


### Deployment
- Make it easy to run xtrace server in rohan-hyd server

### Promotion and marketing
- Hold a DOL session / monthly tech meet session

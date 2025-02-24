### User experience

### Console UI
- Add logs selector
- Add user based login
- Restructure configs data with following schema
    - users
        - <UserID>
            - configs
                - <ConfigID>
                    - runs
                        - <RunID>
                            - logs.log
                            - finalUrl.txt
                            - runConfig.json5 

### Distribution and adoption
- Create infra for xtrace-core where people can redistribute / use xtrace, recieve updates, etc
- Try user guide / infra on a new devbox machine
- Create a demo video showcasing how to use xtrace-core
- Create a single line script which automates cloning, installing deps, opening vscode

### Code parsing and injection logic
- Fix error where a To_string method can cause infinite recursion. #P0
    - e.g. in bool IsEditable(const Node &node) {

#### CPP vars capturing scenarios
- Capture vars declared with "&"
    ```cpp
        const PositionInFlatTree& anchor_position =
        original_anchor_in_flat_tree_.GetPosition();
    ```

### UT and tests
- Add more unit tests for cpp hooks injector to protect against refactor
- Add command to update expected snapshots from tmp generated files

### E2E testing

### Technical debt
- Refactor cpp hooks injector index.ts

### Inner dev loop


### Deployment
- Make it easy to run xtrace server in rohan-hyd server

### Promotion and marketing
- Hold a DOL session / monthly tech meet session

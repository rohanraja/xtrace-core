workspace {

    model {
        dev = person "Software Developer" {
            tags "Customer"
            description "Member in a software dev team, seeking insights into their application code using xTrace"
        }
        admin = person "Software Admin" {
            description "Owner of software app team, will onboard their code repo to xTrace to start tracing"
            tags "Customer"
        }
        xTrace = softwareSystem "xTrace" {
            description "Views code replays, debug recordings"

            webApplication = container "Web Application" "Provides static content and single page application resources to web" "NodeJS" "Web Browser"
            singlePageApplication = container "Single Page Application" "xTrace functionality to users via web browser" "Typescript and React and Redux" "Web Browser"
            webDatabase = container "Web Database" "Stores user accounts, billing, plans, payment data" "Postgres" "Database"
            apiApplication = container "API Application" "Provides backend functionality to web clients using HTTPS/JSON" "Ruby on Rails" {

                signinController = component "Sign In Controller" "Allows users to sign in to the Internet Banking System." "Rails Controller"
                accountsSummaryController = component "Accounts Summary Controller" "Provides customers with a summary of their bank accounts." "Rails Controller"
                resetPasswordController = component "Reset Password Controller" "Allows users to reset their passwords with a single use URL." "Rails Controller"
                securityComponent = component "Security Component" "Provides functionality related to signing in, changing passwords, etc." "Rails Component"
                emailComponent = component "E-mail Component" "Sends e-mails to users." "Rails Component"

                userModel = component "User Model" "Data related to a user record - email, encrypted password" "Rails Model" "Rails Model"

                signinController -> securityComponent "Uses"
                securityComponent -> userModel "uses"
                userModel -> webDatabase "Reads from and writes to" "JSON"

 
            }
            messageQueue = container "Message Queue" "Holds live code running information, batch requests" "Redis" "Queue" {
            }

            codeRecordingsDB = container "Code Recordings DB" "Stores processed code run recordings" "MongoDB" "Database"

            codeRepoOnboardingService = container "Code Repo Onboarding Service" "Onboards code repo to xTrace" "Python" {
            }

            codeInstrumenter = container "Code instrumenter" "Parses source code and adds instrumented code and logs to capture runtime data" ".Net/NodeJS" {
            }

            webApplication -> singlePageApplication "Delivers to customer's web browser"
            singlePageApplication -> apiApplication "Makes API calls to" "JSON/HTTP"
            # apiApplication -> webDatabase "Reads from and writes to" "JSON"
            apiApplication -> messageQueue "Reads from and writes to" "JSON"
            apiApplication -> codeRecordingsDB "Reads from and writes to" "JSON"
            codeRepoOnboardingService -> codeRecordingsDB "Reads from and writes to" "JSON"

            codeRepoOnboardingService -> codeInstrumenter "Invokes" "CLI"
            codeRepoOnboardingService -> messageQueue "Processes requests to onboard code repo"


        }

        customerApplication = softwareSystem "Customer application with xTrace" {
            tags "Customer" "App"
            description "Customer's application with xTrace onboarded"
        }

        customerCode = softwareSystem "Customer application code repo" {
            tags "Customer" "CodeRepo"
            description "Customer's application code repository folder"
        }

        payments = softwareSystem "Payments" {
            tags "Existing System"
            description "Manages direct debits, card payments"
        }

        oauthIdentityProvider = softwareSystem "Oauth Identity Provider" {
            tags "Existing System"
            description "Authenticates user using their Google, Microsoft, Github accounts"
        }


        dev -> xTrace "Views code replays, debug recordings using"
        admin -> xTrace "Onboards xTrace to application code"

        dev -> webApplication "Visits xTrace.io" "HTTPS"
        dev -> singlePageApplication "Views code replays, debug recordings using" "Web browser"
        singlePageApplication -> payments "Redirects for payment to" "HTTPS"
        apiApplication -> payments "Triggers payments using" "JSON/HTTP"
        customerApplication -> apiApplication "Send code run events to" "JSON/HTTPS"
        singlePageApplication -> oauthIdentityProvider "Login/Register account using" "HTTPS"

        admin -> singlePageApplication "Register code repo with access token, code hooking config" "Web UI"
        codeInstrumenter -> customerCode "Reads and writes to" "FileSystem"
    }

    views {
        systemContext xTrace "Diagram1" {
            include *
            autolayout lr
        }

        container xTrace "Containers" {
            include *
            # autoLayout
            description "The container diagram for the xTrace System."
        }

        component apiApplication "Components" {
            include *
            autolayout lr
        }

    styles {
            element "Person" {
                color #ffffff
                fontSize 22
                shape Person
            }
            element "Customer" {
                background #08427b
            }
            element "Bank Staff" {
                background #999999
            }
            element "Software System" {
                background #1168bd
                color #ffffff
            }
            element "Existing System" {
                background #999999
                color #ffffff
            }
            element "Container" {
                background #438dd5
                color #ffffff
            }
            element "Web Browser" {
                shape WebBrowser
            }
            element "Mobile App" {
                shape MobileDeviceLandscape
            }
            element "Database" {
                shape Cylinder
            }
            element "Component" {
                background #85bbf0
                color #000000
            }
            element "Failover" {
                opacity 25
            }
            element "CodeRepo" {
                shape Folder
            }
            element "Rails Model" {
                background #7bc40e 
            }

            element "App" {
                shape RoundedBox
            }
        }
    }

    configuration {
        scope softwaresystem
    }


}
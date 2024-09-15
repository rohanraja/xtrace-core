## Introduction

Code is the best documentation. But connecting the dots between various functions, classes and files can be tedious and time consuming.

With xTrace, you can create debug recordings of your code as your program executes. With those recordings, you can:

- see line by line execution of source code

- observe how a variable changes between statements

- explore the call stack for the most important methods

- add breakpoints and fast forward to them

All just like any code debugger but with additional capability to go back in time and debugging without having to run the actual program.

No worries of mistakenly over stepping while debugging! No hassle to setup a development environment!

Using the recorded code run information, this tool can auto generate sequence diagrams to provide a one page overview of the whole run - an invaluable documentation asset.

### How can this help my software engineering team ?

1.) Can speed-en up the code Ramp up processes for new developers - new folks can quickly get familiar with how the code execution flows, method calling routes - by playing around with the debug recordings and exploring freely.

2.) It will save your customer's time during in-production issues - Live code debugging can be time consuming. With xTrace, you can enable a recording mode in customer's deployment and ask them to provide a recording file when they encounter issues. That xTrace' recording file will have all the information that you would have wanted during live debugging - all without spending time with the customer to debug.

Currently supported languages are C# and TypeScript but the code is open for extensibility with new languages

## How it works

This works by generating a parallel version of original source code by adding code hooks before each line in the source code. Those code hooks send code running information to a Redis server, which accumulates the hook data from all line executions and consolidates it into a single code recording. That recording is viewed in a browser based debugging UI built in React-Redux which communicates with an API built in Ruby on Rails.

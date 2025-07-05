#!/usr/bin/env ts-node

const fs = require('fs');
const path = require('path');

async function testCppHooksInjectorAPI() {
    const baseUrl = 'http://localhost:3001';
    
    // Read the test C++ file
    const testFilePath = path.join(__dirname, 'tests', 'inp.cc');
    const sourceCode = fs.readFileSync(testFilePath, 'utf-8');
    
    console.log('🚀 Testing C++ Hooks Injector HTTP API');
    console.log('=' .repeat(50));
    
    try {
        // Test 1: Health check
        console.log('\n📋 Test 1: Health Check');
        const healthResponse = await fetch(`${baseUrl}/health`);
        if (!healthResponse.ok) {
            throw new Error(`Health check failed: ${healthResponse.status}`);
        }
        const healthData = await healthResponse.json();
        console.log('✅ Health check passed:', healthData);
        
        // Test 2: JSON payload injection
        console.log('\n📋 Test 2: JSON Payload Injection');
        const jsonPayload = {
            code: sourceCode,
            filename: 'inp.cc'
        };
        
        const jsonResponse = await fetch(`${baseUrl}/inject`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(jsonPayload)
        });
        
        if (!jsonResponse.ok) {
            throw new Error(`JSON injection failed: ${jsonResponse.status} ${jsonResponse.statusText}`);
        }
        
        const jsonResult = await jsonResponse.json();
        console.log('✅ JSON injection success');
        console.log(`   - Injector used: ${jsonResult.injectorUsed}`);
        console.log(`   - Has errors: ${jsonResult.hasError}`);
        console.log(`   - Original lines: ${jsonResult.statistics.originalLines}`);
        console.log(`   - Modified lines: ${jsonResult.statistics.modifiedLines}`);
        console.log(`   - Size change: ${jsonResult.statistics.originalSize} → ${jsonResult.statistics.modifiedSize} bytes`);
        
        // Save the result for inspection
        const outputPath = path.join(__dirname, 'tests', 'http_output.cc');
        fs.writeFileSync(outputPath, jsonResult.modifiedCode);
        console.log(`   - Output saved to: ${outputPath}`);
        
        // Test 3: Plain text payload injection
        console.log('\n📋 Test 3: Plain Text Payload Injection');
        const textResponse = await fetch(`${baseUrl}/inject?filename=test.cc`, {
            method: 'POST',
            headers: {
                'Content-Type': 'text/plain'
            },
            body: sourceCode
        });
        
        if (!textResponse.ok) {
            throw new Error(`Text injection failed: ${textResponse.status} ${textResponse.statusText}`);
        }
        
        const textResult = await textResponse.json();
        console.log('✅ Plain text injection success');
        console.log(`   - Injector used: ${textResult.injectorUsed}`);
        console.log(`   - Has errors: ${textResult.hasError}`);
        console.log(`   - Original lines: ${textResult.statistics.originalLines}`);
        console.log(`   - Modified lines: ${textResult.statistics.modifiedLines}`);
        
        // Test 4: Error handling - empty code
        console.log('\n📋 Test 4: Error Handling - Empty Code');
        const emptyResponse = await fetch(`${baseUrl}/inject`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ code: '' })
        });
        
        if (emptyResponse.status !== 400) {
            console.log('⚠️  Expected 400 error for empty code, got:', emptyResponse.status);
        } else {
            console.log('✅ Properly rejected empty code with 400 error');
        }
        
        // Test 5: Error handling - invalid JSON
        console.log('\n📋 Test 5: Error Handling - Invalid JSON');
        const invalidResponse = await fetch(`${baseUrl}/inject`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ invalidField: sourceCode })
        });
        
        if (invalidResponse.status !== 400) {
            console.log('⚠️  Expected 400 error for invalid JSON, got:', invalidResponse.status);
        } else {
            console.log('✅ Properly rejected invalid JSON with 400 error');
        }
        
        // Performance test
        console.log('\n📋 Test 6: Performance Test (10 requests)');
        const startTime = Date.now();
        const promises = [];
        
        for (let i = 0; i < 10; i++) {
            const promise = fetch(`${baseUrl}/inject`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    code: sourceCode,
                    filename: `test_${i}.cc`
                })
            });
            promises.push(promise);
        }
        
        const responses = await Promise.all(promises);
        const endTime = Date.now();
        
        const allSuccessful = responses.every(r => r.ok);
        console.log(`✅ Performance test completed`);
        console.log(`   - All requests successful: ${allSuccessful}`);
        console.log(`   - Total time: ${endTime - startTime}ms`);
        console.log(`   - Average per request: ${(endTime - startTime) / 10}ms`);
        
        console.log('\n🎉 All tests completed successfully!');
        console.log('=' .repeat(50));
        
        // Display first few lines of the modified code
        console.log('\n📝 Sample of modified code (first 20 lines):');
        console.log('-' .repeat(40));
        const modifiedLines = jsonResult.modifiedCode.split('\n').slice(0, 20);
        modifiedLines.forEach((line, index) => {
            console.log(`${(index + 1).toString().padStart(3)}: ${line}`);
        });
        if (jsonResult.modifiedCode.split('\n').length > 20) {
            console.log('... (truncated)');
        }
        
    } catch (error) {
        console.error('\n❌ Test failed:', error);
        process.exit(1);
    }
}

// Check if server is running and start tests
async function main() {
    const baseUrl = 'http://localhost:3001';
    
    try {
        console.log('🔍 Checking if server is running...');
        const response = await fetch(`${baseUrl}/health`);
        if (response.ok) {
            console.log('✅ Server is running, starting tests...');
            await testCppHooksInjectorAPI();
        } else {
            throw new Error('Server not responding properly');
        }
    } catch (error) {
        console.error('\n❌ Server is not running!');
        console.error('Please start the server first with: npm run serve');
        console.error('Then run this test with: npm run test-serve');
        process.exit(1);
    }
}

if (require.main === module) {
    main();
}

module.exports = { testCppHooksInjectorAPI };

# C++ Hooks Injector HTTP API

This directory provides both a command-line interface and an HTTP API for injecting xTrace hooks into C++ code.

## Files

- `index.ts` - Original command-line interface (stdin/stdout)
- `serve.ts` - HTTP server providing REST API
- `test_serve.ts` - Test suite for the HTTP API

## HTTP API Usage

### Starting the Server

```bash
npm run serve
```

The server runs on port 3001 by default (configurable via PORT environment variable).

### API Endpoints

#### Health Check
```
GET /health
```

Returns server status and timestamp.

#### Code Injection
```
POST /inject
```

**Request Body Options:**

1. **JSON format:**
```json
{
  "code": "// Your C++ code here",
  "filename": "optional_filename.cc",
  "codeVersion": "optional_version_id",
  "skipVariablesHooking": false
}
```

2. **Plain text format:**
```
Content-Type: text/plain

// Your C++ code here
```

**Parameters:**
- `code` (required): C++ source code to inject hooks into
- `filename` (optional): Name of the file (defaults to "input.cc")
- `codeVersion` (optional): Version identifier for the code
- `skipVariablesHooking` (optional): Set to `true` to skip variable tracking (LocalVarUpdate calls) while preserving method entry and line logging. Defaults to `false`.

For plain text requests, you can pass `skipVariablesHooking` as a query parameter:
```
POST /inject?skipVariablesHooking=true
```

**Response:**
```json
{
  "success": true,
  "injectorUsed": "tree-sitter",
  "hasError": false,
  "originalCode": "...",
  "modifiedCode": "...",
  "filename": "input.cc",
  "codeVersion": "",
  "skipVariablesHooking": false,
  "timestamp": "2025-07-05T08:40:13.067Z",
  "statistics": {
    "originalLines": 579,
    "modifiedLines": 1035,
    "originalSize": 19589,
    "modifiedSize": 38791
  }
}
```

**Response Fields:**
- `success`: Boolean indicating if injection was successful
- `injectorUsed`: Which injector was used ("tree-sitter", "clang", or "tree-sitter-fallback")
- `hasError`: Boolean indicating if the generated code has syntax errors
- `originalCode`: The input C++ code
- `modifiedCode`: The C++ code with xTrace hooks injected
- `filename`: Name of the processed file
- `codeVersion`: Version identifier of the code
- `skipVariablesHooking`: Whether variable hooking was skipped for this request
- `timestamp`: When the processing was completed
- `statistics`: Object containing line counts and file sizes

### Variable Hooking Behavior

The `skipVariablesHooking` parameter controls the type of xTrace hooks that are injected:

**When `skipVariablesHooking: false` (default):**
- Injects `OnMethodEnter` calls for method entry logging
- Injects `LogLineRun` calls for line-by-line execution tracking  
- Injects `LocalVarUpdate` calls for variable tracking (parameters and local variables)
- Provides comprehensive execution tracing

**When `skipVariablesHooking: true`:**
- Injects `OnMethodEnter` calls for method entry logging
- Injects `LogLineRun` calls for line-by-line execution tracking
- **Skips** `LocalVarUpdate` calls for variable tracking
- Provides method flow and line execution tracking without variable details
- Offers better performance for large files when variable tracking is not needed

### Testing

Run the comprehensive test suite:

```bash
npm run test-serve
```

This will test:
- Health check endpoint
- JSON payload injection
- Plain text payload injection
- Error handling (empty code, invalid JSON)
- Performance testing (10 concurrent requests)

### Environment Configuration

The server respects the same environment variables as the CLI tool:

- `USE_CLANG_INJECTOR=true` - Use Clang LibTooling injector instead of tree-sitter
- `PORT=3001` - Set custom port for the HTTP server

### Performance

The HTTP API processes C++ files efficiently:
- Average processing time: ~184ms per request (tree-sitter injector)
- Supports concurrent requests
- Automatic memory management and cleanup

### Error Handling

The API provides comprehensive error handling:
- 400 Bad Request for invalid input
- 500 Internal Server Error for processing failures
- Automatic fallback from Clang to tree-sitter injector
- Detailed error messages and timestamps

### Example Usage

#### Using curl

```bash
# Health check
curl http://localhost:3001/health

# Inject hooks (JSON)
curl -X POST http://localhost:3001/inject \
  -H "Content-Type: application/json" \
  -d '{"code": "#include <iostream>\nint main() { return 0; }", "filename": "test.cc"}'

# Inject hooks with variable hooking disabled (JSON)
curl -X POST http://localhost:3001/inject \
  -H "Content-Type: application/json" \
  -d '{"code": "#include <iostream>\nint main() { return 0; }", "filename": "test.cc", "skipVariablesHooking": true}'

# Inject hooks (plain text)
curl -X POST http://localhost:3001/inject \
  -H "Content-Type: text/plain" \
  -d '#include <iostream>
int main() { return 0; }'

# Inject hooks with variable hooking disabled (plain text with query param)
curl -X POST "http://localhost:3001/inject?skipVariablesHooking=true" \
  -H "Content-Type: text/plain" \
  -d '#include <iostream>
int main() { return 0; }'
```

#### Using JavaScript/Node.js

```javascript
// Basic injection
const response = await fetch('http://localhost:3001/inject', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    code: '#include <iostream>\nint main() { return 0; }',
    filename: 'test.cc'
  })
});

const result = await response.json();
console.log('Modified code:', result.modifiedCode);

// Injection with variable hooking disabled for better performance
const performanceResponse = await fetch('http://localhost:3001/inject', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    code: '#include <iostream>\nint main() { return 0; }',
    filename: 'test.cc',
    skipVariablesHooking: true
  })
});

const performanceResult = await performanceResponse.json();
console.log('Skip variables hooking enabled:', performanceResult.skipVariablesHooking);
```

### Integration

The HTTP API can be easily integrated into:
- VS Code extensions
- CI/CD pipelines  
- Web-based code editors
- Build systems
- Development tools

Both the CLI (`index.ts`) and HTTP API (`serve.ts`) use the same underlying injection logic, ensuring consistent results across interfaces.

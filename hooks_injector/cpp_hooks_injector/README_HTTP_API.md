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
  "filename": "optional_filename.cc"
}
```

2. **Plain text format:**
```
Content-Type: text/plain

// Your C++ code here
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
  "timestamp": "2025-07-05T08:40:13.067Z",
  "statistics": {
    "originalLines": 579,
    "modifiedLines": 1035,
    "originalSize": 19589,
    "modifiedSize": 38791
  }
}
```

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

# Inject hooks (plain text)
curl -X POST http://localhost:3001/inject \
  -H "Content-Type: text/plain" \
  -d '#include <iostream>
int main() { return 0; }'
```

#### Using JavaScript/Node.js

```javascript
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
```

### Integration

The HTTP API can be easily integrated into:
- VS Code extensions
- CI/CD pipelines  
- Web-based code editors
- Build systems
- Development tools

Both the CLI (`index.ts`) and HTTP API (`serve.ts`) use the same underlying injection logic, ensuring consistent results across interfaces.

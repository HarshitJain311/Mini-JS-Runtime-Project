# Mini JavaScript Runtime

A production-structured web app that lets you upload a `.js` file, preview it, and run it on a C++ backend. The C++ runtime layer executes the uploaded JavaScript **via Node.js internally**, capturing `stdout` and `stderr` and returning JSON results.

## Features
- Drag & drop `.js` upload (extension-validated)
- File size limit (200 KB)
- Read-only code preview (VS Code-like dark theme)
- Run button with loading spinner + disabled state
- Terminal-style output console (green text on black)
- Error display section (red text)
- Execution timeout protection (5 seconds)
- Temp-file cleanup after execution
- No arbitrary shell command execution (uses `CreateProcess` with fixed args)

## Tech Stack
- **Frontend:** HTML5, CSS3, Vanilla JavaScript
- **Backend:** C++17 (HTTP server + upload handling)
- **Execution engine:** Node.js (invoked by C++ using `node <tempFile>.js`)

## Project Structure
```
project/
├── backend/
│   ├── main.cpp
│   ├── server.cpp
│   ├── runtime.cpp
│   ├── runtime.h
│   ├── uploads/
│   ├── temp/
│   └── CMakeLists.txt
├── frontend/
│   ├── index.html
│   ├── style.css
│   ├── script.js
│   └── assets/
├── README.md
└── sample_test.js
```

## Requirements
- C++ compiler supporting C++17 (MSVC on Windows)
- CMake
- Node.js installed and available in `PATH`

## Build Instructions (CMake + g++)
### Windows (MSVC)
```bash
cd project/backend
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Run Instructions
1. Start the backend:
```bash
cd project/backend/build
mini_js_runtime_backend.exe
```
2. Open the frontend directly:
- Open `project/frontend/index.html` in your browser.

> The frontend calls `http://localhost:8080/run`.

## API Documentation
### `POST /run`
Uploads a file using `multipart/form-data`.

**Request**
- Field name: `file`
- Content type: the browser will set appropriate multipart boundaries
- Accepted: `.js` files only

**Success Response (HTTP 200)**
```json
{ "success": true, "output": "7 is Odd" }
```

**Error Response (HTTP 200)**
```json
{ "success": false, "error": "ReferenceError: x is not defined" }
```

## Screenshots
Add screenshots after running:
- Upload area + preview
- Console output success
- Console output error

## Future Improvements
- Replace the minimal multipart parser with a robust library
- Serve frontend assets from backend (optional)
- Add per-request isolation (container/sandbox)
- Support larger files with streaming
- Improve syntax highlighting


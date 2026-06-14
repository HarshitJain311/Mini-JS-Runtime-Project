# Quick Run (Windows) — Mini JavaScript Runtime

## 0) Install prerequisites
- **CMake**
- **C++ build tools** (MSVC / Visual Studio Build Tools)
- **Node.js** and ensure `node` works in CMD:
  ```bat
  node -v
  ```

## 1) Build backend (C++ server)
Open **Command Prompt** in `C:/Users/DEll/Desktop` and run:
```bat
cd project\backend
if not exist build mkdir build
cd build
cmake ..
cmake --build . --config Release
```

You should get:
- `project\backend\build\mini_js_runtime_backend.exe`

## 2) Start backend
In the same CMD window (or new one):
```bat
cd project\backend\build
mini_js_runtime_backend.exe
```

Backend listens on:
- `http://localhost:8081/run`

## 3) Run frontend
Open in your browser:
- `project\frontend\index.html`

Then:
- Drag & drop a `.js` file (max **200 KB**) into the upload area
- Click **Run Code**

## 4) Test with provided sample
- Open `project/frontend/index.html`
- Upload `project/sample_test.js`

Expected output:
- `7 is Odd`


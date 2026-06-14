#pragma once

#include <string>

struct RuntimeResult {
    bool success = false;
    std::string output;
    std::string error;
};

// Executes a JavaScript file using Node.js.
// - filePath: path to the JS file
// - timeoutMs: maximum runtime duration
RuntimeResult executeNodeFile(const std::string& filePath, int timeoutMs);


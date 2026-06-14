// #include "httplib.h"
// #include "runtime.h"

// #include <iostream>
// #include <fstream>
// #include <cstdio>

// using namespace httplib;
// using namespace std;

// int main() {

//     Server svr;

//     // CORS
//     svr.set_default_headers({
//         {"Access-Control-Allow-Origin", "*"},
//         {"Access-Control-Allow-Headers", "*"},
//         {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"}
//     });

//     // OPTIONS request
//     svr.Options(R"(.*)", [](const Request&, Response& res) {
//         res.status = 200;
//     });

//     // POST /run
//     svr.Post("/run", [](const Request& req, Response& res) {

//         // Check file exists
//         if (!req.has_file("file")) {

//             res.status = 400;

//             res.set_content(
//                 R"({"success":false,"error":"No file uploaded"})",
//                 "application/json"
//             );

//             return;
//         }

//         // Get uploaded file
//         auto file = req.get_file_value("file");

//         // Validate extension
//         if (file.filename.find(".js") == string::npos) {

//             res.status = 400;

//             res.set_content(
//                 R"({"success":false,"error":"Only .js files allowed"})",
//                 "application/json"
//             );

//             return;
//         }

//         // Save temp file
//         string tempFile = "temp.js";

//         ofstream out(tempFile, ios::binary);

//         if (!out.is_open()) {

//             res.status = 500;

//             res.set_content(
//                 R"({"success":false,"error":"Cannot create temp file"})",
//                 "application/json"
//             );

//             return;
//         }

//         out << file.content;

//         out.close();

//         // Execute JS
//         RuntimeResult result =
//             executeNodeFile(tempFile, 5000);

//         remove(tempFile.c_str());

//         string json;

//         if (result.success) {

//             json =
//                 "{\"success\":true,\"output\":\"" +
//                 result.output +
//                 "\"}";
//         }
//         else {

//             json =
//                 "{\"success\":false,\"error\":\"" +
//                 result.error +
//                 "\"}";
//         }

//         res.set_content(
//             json,
//             "application/json"
//         );
//     });

//     cout << "Server listening on port 8081..." << endl;

//     svr.listen("0.0.0.0", 8081);

//     return 0;
// }

#include "server.h"
#include "httplib.h"
#include "runtime.h"

#include <iostream>
#include <fstream>
#include <cstdio>
#include <chrono>
#include <random>
#include <cctype>

using namespace std;


static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char ch : s) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch < 0x20) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", ch);
                    out += buf;
                } else {
                    out += static_cast<char>(ch);
                }
        }
    }
    return out;
}

static bool endsWithJs(const std::string& filename) {
    if (filename.size() < 3) return false;
    auto dotPos = filename.rfind('.');
    if (dotPos == std::string::npos) return false;
    std::string ext = filename.substr(dotPos);
    for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext == ".js";
}

static std::string makeTempJsFilename() {
    using namespace std::chrono;
    const auto now = high_resolution_clock::now().time_since_epoch();
    const auto ns = duration_cast<nanoseconds>(now).count();
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    return "temp_" + std::to_string(ns) + "_" + std::to_string(dist(gen)) + ".js";
}

int Server::run() {
    httplib::Server svr;
    svr.set_payload_max_length(200 * 1024);




    // CORS
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Headers", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"}
    });

    // OPTIONS request
    svr.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 200;
    });


    svr.set_payload_max_length(200 * 1024);

    // Ensure multipart/form-data is parsed for file upload.
    // cpp-httplib stores uploaded files in req.form.files.

    svr.Post("/run", [](const httplib::Request& req, httplib::Response& res) {
        auto hasFile = req.form.has_file("file");

        if (!hasFile) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"No file uploaded"})", "application/json");
            return;
        }

        auto file = req.form.get_file("file");
        if (file.filename.empty() || file.content.empty()) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"No file uploaded"})", "application/json");
            return;
        }

        if (!endsWithJs(file.filename)) {
            res.status = 400;
            res.set_content(R"({"success":false,"error":"Only .js files allowed"})", "application/json");
            return;
        }

        const std::string tempFile = makeTempJsFilename();
        {
            std::ofstream out(tempFile, std::ios::binary);
            if (!out.is_open()) {
                res.status = 500;
                res.set_content(R"({"success":false,"error":"Cannot create temp file"})", "application/json");
                return;
            }
            out << file.content;
        }

        RuntimeResult result = executeNodeFile(tempFile, 5000);
        std::remove(tempFile.c_str());

        if (result.success) {
            std::string json = "{\"success\":true,\"output\":\"" + jsonEscape(result.output) + "\"}";
            res.set_content(json, "application/json");
        } else {
            std::string json = "{\"success\":false,\"error\":\"" + jsonEscape(result.error) + "\"}";
            res.set_content(json, "application/json");
        }
    });


    cout << "Server listening on port 8081..." << endl;

    svr.listen("0.0.0.0", 8081);


    return 0;
}
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <algorithm>

#include "utils/Utils.h"
#include "utils/MountedPartitions.h"
#include "utils/Session.h"
#include "commands/MkDisk.h"
#include "commands/RmDisk.h"
#include "commands/FDisk.h"
#include "commands/Mount.h"
#include "commands/MkFs.h"
#include "commands/Login.h"

#define PORT 3001
#define BUFFER_SIZE 65536

// -----------------------------------------------
// Procesa un solo comando y retorna su salida
// -----------------------------------------------
std::string processCommand(const std::string& rawLine) {
    std::string line = trim(rawLine);
    if(line.empty())    return "";
    if(line[0] == '#')  return line; // comentario

    // Separar nombre del comando del resto
    size_t spacePos = line.find(' ');
    std::string cmd = toLower(
        (spacePos == std::string::npos) ? line : line.substr(0, spacePos)
    );
    std::string rest = (spacePos == std::string::npos) ? "" : line.substr(spacePos + 1);

    auto params = parseParams(rest);

    if(cmd == "mkdisk")  return MkDisk::execute(params);
    if(cmd == "rmdisk")  return RmDisk::execute(params);
    if(cmd == "fdisk")   return FDisk::execute(params);
    if(cmd == "mount")   return Mount::execute(params);
    if(cmd == "mounted") return Mounted::execute();
    if(cmd == "mkfs")    return MkFs::execute(params);
    if(cmd == "login")   return Login::execute(params);
    if(cmd == "logout")  return Logout::execute();

    return "Error: Comando no reconocido -> " + cmd;
}

// -----------------------------------------------
// Procesa múltiples comandos (script completo)
// -----------------------------------------------
std::string processScript(const std::string& script) {
    std::string output = "";
    std::istringstream ss(script);
    std::string line;

    while(std::getline(ss, line)) {
        std::string result = processCommand(line);
        if(!result.empty()) {
            output += result + "\n";
        }
    }
    return output;
}

// -----------------------------------------------
// Construir respuesta HTTP con CORS
// -----------------------------------------------
std::string buildResponse(const std::string& body,
                           const std::string& status = "200 OK") {
    std::string response =
        "HTTP/1.1 " + status + "\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n" + body;
    return response;
}

// -----------------------------------------------
// Escapar string para JSON
// -----------------------------------------------
std::string jsonEscape(const std::string& s) {
    std::string result;
    for(char c : s) {
        if(c == '"')  result += "\\\"";
        else if(c == '\\') result += "\\\\";
        else if(c == '\n') result += "\\n";
        else if(c == '\r') result += "\\r";
        else if(c == '\t') result += "\\t";
        else result += c;
    }
    return result;
}

// -----------------------------------------------
// Extraer body de la petición HTTP
// -----------------------------------------------
std::string extractBody(const std::string& request) {
    size_t pos = request.find("\r\n\r\n");
    if(pos == std::string::npos) return "";
    return request.substr(pos + 4);
}

// -----------------------------------------------
// Extraer valor de campo JSON simple
// {"commands":"valor"} -> valor
// -----------------------------------------------
std::string extractJsonField(const std::string& json,
                              const std::string& field) {
    std::string key = "\"" + field + "\"";
    size_t pos = json.find(key);
    if(pos == std::string::npos) return "";

    pos = json.find(":", pos + key.size());
    if(pos == std::string::npos) return "";
    pos++;

    // Saltar espacios
    while(pos < json.size() && json[pos] == ' ') pos++;
    if(pos >= json.size()) return "";

    if(json[pos] == '"') {
        // String entre comillas
        pos++;
        std::string value;
        while(pos < json.size() && json[pos] != '"') {
            if(json[pos] == '\\' && pos + 1 < json.size()) {
                char next = json[pos+1];
                if(next == 'n')       { value += '\n'; pos += 2; }
                else if(next == 'r')  { value += '\r'; pos += 2; }
                else if(next == 't')  { value += '\t'; pos += 2; }
                else if(next == '"')  { value += '"';  pos += 2; }
                else if(next == '\\') { value += '\\'; pos += 2; }
                else { value += json[pos++]; }
            } else {
                value += json[pos++];
            }
        }
        return value;
    }
    return "";
}

// -----------------------------------------------
// Manejar una conexión de cliente
// -----------------------------------------------
void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE] = {0};
    read(clientSocket, buffer, BUFFER_SIZE - 1);
    std::string request(buffer);

    // Extraer método y path
    std::string method = request.substr(0, request.find(' '));
    size_t pathStart   = request.find(' ') + 1;
    size_t pathEnd     = request.find(' ', pathStart);
    std::string path   = request.substr(pathStart, pathEnd - pathStart);

    std::string response;

    // Manejar preflight CORS
    if(method == "OPTIONS") {
        response = buildResponse("", "204 No Content");
        send(clientSocket, response.c_str(), response.size(), 0);
        close(clientSocket);
        return;
    }

    // -----------------------------------------------
    // POST /execute -> ejecutar comandos
    // -----------------------------------------------
    if(method == "POST" && path == "/execute") {
        std::string body     = extractBody(request);
        std::string commands = extractJsonField(body, "commands");

        if(commands.empty()) {
            std::string json = "{\"output\":\"Error: No se enviaron comandos\"}";
            response = buildResponse(json);
        } else {
            std::string output = processScript(commands);
            std::string json   = "{\"output\":\"" + jsonEscape(output) + "\"}";
            response = buildResponse(json);
        }
    }
    // -----------------------------------------------
    // GET /status -> estado del servidor
    // -----------------------------------------------
    else if(method == "GET" && path == "/status") {
        std::string session = currentSession.active
            ? currentSession.username : "none";
        std::string json =
            "{\"status\":\"running\","
            "\"session\":\"" + session + "\","
            "\"mounted\":" + std::to_string(MountedPartitions::mounted.size()) + "}";
        response = buildResponse(json);
    }
    else {
        response = buildResponse("{\"error\":\"Ruta no encontrada\"}", "404 Not Found");
    }

    send(clientSocket, response.c_str(), response.size(), 0);
    close(clientSocket);
}

// -----------------------------------------------
// MAIN
// -----------------------------------------------
int main() {
    srand(time(nullptr));

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == 0) {
        std::cerr << "Error creando socket" << std::endl;
        return 1;
    }

    // Permitir reutilizar el puerto
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(PORT);

    if(bind(serverSocket, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Error en bind" << std::endl;
        return 1;
    }

    if(listen(serverSocket, 10) < 0) {
        std::cerr << "Error en listen" << std::endl;
        return 1;
    }

    std::cout << "Servidor corriendo en puerto " << PORT << std::endl;

    while(true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if(clientSocket < 0) continue;
        handleClient(clientSocket);
    }

    return 0;
}
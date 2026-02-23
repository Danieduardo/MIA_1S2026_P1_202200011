#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>

// Convierte string a minúsculas
inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// Trim de espacios
inline std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    if(start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

// Crea directorios recursivamente (como mkdir -p)
inline bool mkdirRecursive(const std::string& path) {
    std::string current;
    std::istringstream ss(path);
    std::string token;
    
    // Manejo de ruta absoluta
    if(path[0] == '/') current = "/";
    
    std::vector<std::string> parts;
    std::stringstream ss2(path);
    while(std::getline(ss2, token, '/')) {
        if(!token.empty()) parts.push_back(token);
    }
    
    for(const auto& part : parts) {
        current += part + "/";
        struct stat st;
        if(stat(current.c_str(), &st) != 0) {
            if(mkdir(current.c_str(), 0755) != 0) {
                return false;
            }
        }
    }
    return true;
}

// Obtiene el directorio padre de una ruta
inline std::string getParentDir(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if(pos == std::string::npos || pos == 0) return "/";
    return path.substr(0, pos);
}

// Obtiene el nombre del archivo de una ruta
inline std::string getFileName(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if(pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

// Parsea los parámetros de un comando
// Ejemplo: mkdisk -size=10 -unit=M -path=/home/disco.mia
// Retorna map de {param -> valor}
inline std::vector<std::pair<std::string,std::string>> parseParams(const std::string& line) {
    std::vector<std::pair<std::string,std::string>> params;
    std::string current = line;
    
    size_t i = 0;
    while(i < current.size()) {
        // Saltar espacios
        while(i < current.size() && current[i] == ' ') i++;
        if(i >= current.size()) break;
        
        if(current[i] == '-') {
            // Encontrar el parámetro
            size_t start = i;
            i++;
            std::string key, val;
            
            // Leer hasta '=' o espacio
            while(i < current.size() && current[i] != '=' && current[i] != ' ') {
                key += current[i++];
            }
            
            if(i < current.size() && current[i] == '=') {
                i++; // saltar '='
                // Leer valor (con soporte de comillas)
                if(i < current.size() && current[i] == '"') {
                    i++; // saltar comilla inicial
                    while(i < current.size() && current[i] != '"') {
                        val += current[i++];
                    }
                    if(i < current.size()) i++; // saltar comilla final
                } else {
                    while(i < current.size() && current[i] != ' ') {
                        val += current[i++];
                    }
                }
            }
            params.push_back({toLower(key), val});
        } else {
            i++;
        }
    }
    return params;
}

// Formatea tiempo a string legible
inline std::string timeToString(time_t t) {
    char buf[64];
    struct tm* tm_info = localtime(&t);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
    return std::string(buf);
}

#endif // UTILS_H
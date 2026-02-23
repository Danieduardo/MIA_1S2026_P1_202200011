#ifndef RMDISK_H
#define RMDISK_H

#include <string>
#include <vector>
#include <cstdio>
#include <sys/stat.h>
#include "../utils/Utils.h"

class RmDisk {
public:
    static std::string execute(const std::vector<std::pair<std::string,std::string>>& params) {
        std::string path = "";

        for(const auto& p : params) {
            std::string key = toLower(p.first);
            if(key == "path") {
                path = p.second;
            } else {
                return "Error: Parámetro no reconocido -> " + p.first;
            }
        }

        if(path.empty()) {
            return "Error: El parámetro -path es obligatorio";
        }

        // Verificar que el archivo existe
        struct stat st;
        if(stat(path.c_str(), &st) != 0) {
            return "Error: El archivo no existe: " + path;
        }

        // Eliminar el archivo
        if(remove(path.c_str()) != 0) {
            return "Error: No se pudo eliminar el archivo: " + path;
        }

        return "OK: Disco eliminado exitosamente: " + path;
    }
};

#endif // RMDISK_H
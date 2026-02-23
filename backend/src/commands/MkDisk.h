#ifndef MKDISK_H
#define MKDISK_H

#include <string>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include "../structs/Structs.h"
#include "../utils/Utils.h"

class MkDisk {
public:
    static std::string execute(const std::vector<std::pair<std::string,std::string>>& params) {
        // Valores por defecto
        int         size = -1;
        std::string fit  = "ff";  // First Fit por defecto
        std::string unit = "m";   // Megabytes por defecto
        std::string path = "";

        // Leer parámetros
        for(const auto& p : params) {
            std::string key = toLower(p.first);
            std::string val = p.second;

            if(key == "size") {
                size = std::stoi(val);
            } else if(key == "fit") {
                fit = toLower(val);
            } else if(key == "unit") {
                unit = toLower(val);
            } else if(key == "path") {
                path = val;
            } else {
                return "Error: Parámetro no reconocido -> " + p.first;
            }
        }

        // Validaciones
        if(size <= 0) {
            return "Error: El parámetro -size es obligatorio y debe ser mayor a 0";
        }
        if(path.empty()) {
            return "Error: El parámetro -path es obligatorio";
        }
        if(fit != "bf" && fit != "ff" && fit != "wf") {
            return "Error: -fit debe ser BF, FF o WF";
        }
        if(unit != "k" && unit != "m") {
            return "Error: -unit debe ser K o M";
        }

        // Calcular tamaño en bytes
        long long sizeBytes;
        if(unit == "k") {
            sizeBytes = (long long)size * 1024;
        } else {
            sizeBytes = (long long)size * 1024 * 1024;
        }

        // Crear directorios padre si no existen
        std::string parentDir = getParentDir(path);
        if(!mkdirRecursive(parentDir)) {
            return "Error: No se pudo crear el directorio: " + parentDir;
        }

        // Crear el archivo binario del disco
        std::ofstream disk(path, std::ios::binary | std::ios::out);
        if(!disk.is_open()) {
            return "Error: No se pudo crear el archivo en: " + path;
        }

        // Llenar con ceros usando buffer de 1024 bytes
        char buffer[1024];
        std::memset(buffer, 0, sizeof(buffer));
        long long remaining = sizeBytes;
        while(remaining > 0) {
            long long toWrite = (remaining > 1024) ? 1024 : remaining;
            disk.write(buffer, toWrite);
            remaining -= toWrite;
        }

        // Crear y escribir el MBR al inicio del disco
        MBR mbr;
        mbr.mbr_tamano         = (int)sizeBytes;
        mbr.mbr_fecha_creacion = time(nullptr);
        mbr.mbr_dsk_signature  = rand() % 100000;

        // Convertir fit a char
        if(fit == "bf") mbr.dsk_fit = 'B';
        else if(fit == "ff") mbr.dsk_fit = 'F';
        else mbr.dsk_fit = 'W';

        // Escribir MBR al inicio
        disk.seekp(0, std::ios::beg);
        disk.write(reinterpret_cast<char*>(&mbr), sizeof(MBR));
        disk.close();

        return "OK: Disco creado exitosamente en " + path +
               " | Tamaño: " + std::to_string(sizeBytes) + " bytes";
    }
};

#endif // MKDISK_H
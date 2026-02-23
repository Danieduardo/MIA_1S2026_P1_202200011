#ifndef FDISK_H
#define FDISK_H

#include <string>
#include <vector>
#include <fstream>
#include <cstring>
#include <sys/stat.h>
#include "../structs/Structs.h"
#include "../utils/Utils.h"

class FDisk {
public:
    static std::string execute(const std::vector<std::pair<std::string,std::string>>& params) {
        int         size = -1;
        std::string unit = "k";
        std::string path = "";
        std::string type = "p";
        std::string fit  = "wf";
        std::string name = "";

        for(const auto& p : params) {
            std::string key = toLower(p.first);
            std::string val = p.second;

            if(key == "size")      size = std::stoi(val);
            else if(key == "unit") unit = toLower(val);
            else if(key == "path") path = val;
            else if(key == "type") type = toLower(val);
            else if(key == "fit")  fit  = toLower(val);
            else if(key == "name") name = val;
            else return "Error: Parámetro no reconocido -> " + p.first;
        }

        // Validaciones
        if(path.empty()) return "Error: -path es obligatorio";
        if(name.empty()) return "Error: -name es obligatorio";
        if(size <= 0)    return "Error: -size debe ser mayor a 0";

        if(unit != "b" && unit != "k" && unit != "m")
            return "Error: -unit debe ser B, K o M";
        if(type != "p" && type != "e" && type != "l")
            return "Error: -type debe ser P, E o L";
        if(fit != "bf" && fit != "ff" && fit != "wf")
            return "Error: -fit debe ser BF, FF o WF";

        // Verificar que el disco existe
        struct stat st;
        if(stat(path.c_str(), &st) != 0)
            return "Error: El disco no existe: " + path;

        // Calcular tamaño en bytes
        long long sizeBytes;
        if(unit == "b")      sizeBytes = size;
        else if(unit == "k") sizeBytes = (long long)size * 1024;
        else                 sizeBytes = (long long)size * 1024 * 1024;

        char fitChar = (fit == "bf") ? 'B' : (fit == "ff") ? 'F' : 'W';

        // Abrir disco
        std::fstream disk(path, std::ios::binary | std::ios::in | std::ios::out);
        if(!disk.is_open())
            return "Error: No se pudo abrir el disco: " + path;

        // Leer MBR
        MBR mbr;
        disk.seekg(0, std::ios::beg);
        disk.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));

        if(type == "l") {
            return createLogical(disk, mbr, path, sizeBytes, fitChar, name);
        } else {
            return createPrimary(disk, mbr, path, sizeBytes, fitChar, name, type[0]);
        }
    }

private:
    // -----------------------------------------------
    // Crear partición primaria o extendida
    // -----------------------------------------------
    static std::string createPrimary(std::fstream& disk, MBR& mbr,
                                     const std::string& path,
                                     long long sizeBytes, char fitChar,
                                     const std::string& name, char typeChar) {
        // Contar particiones primarias+extendidas actuales
        int count    = 0;
        bool hasExt  = false;
        int  extIdx  = -1;

        for(int i = 0; i < 4; i++) {
            if(mbr.mbr_partitions[i].part_start != -1) {
                count++;
                if(mbr.mbr_partitions[i].part_type == 'E') {
                    hasExt = true;
                    extIdx = i;
                }
                // Verificar nombre duplicado
                if(std::string(mbr.mbr_partitions[i].part_name) == name)
                    return "Error: Ya existe una partición con ese nombre: " + name;
            }
        }

        if(count >= 4)
            return "Error: Ya existen 4 particiones (máximo permitido)";
        if(typeChar == 'E' && hasExt)
            return "Error: Solo puede existir una partición extendida por disco";

        // Encontrar espacio libre según el ajuste del MBR
        int startByte = findFreeSpace(mbr, mbr.mbr_tamano, sizeBytes, mbr.dsk_fit);
        if(startByte == -1)
            return "Error: No hay espacio suficiente en el disco";

        // Encontrar slot libre en el MBR
        int slot = -1;
        for(int i = 0; i < 4; i++) {
            if(mbr.mbr_partitions[i].part_start == -1) {
                slot = i; break;
            }
        }

        // Llenar la partición
        Partition& part = mbr.mbr_partitions[slot];
        part.part_status = '0';
        part.part_type   = (typeChar == 'p' || typeChar == 'P') ? 'P' : 'E';
        part.part_fit    = fitChar;
        part.part_start  = startByte;
        part.part_s      = (int)sizeBytes;
        std::strncpy(part.part_name, name.c_str(), 15);
        part.part_name[15]   = '\0';
        part.part_correlative = -1;
        std::memset(part.part_id, 0, 4);

        // Si es extendida, crear el primer EBR
        if(typeChar == 'E' || typeChar == 'e') {
            EBR ebr;
            ebr.part_mount = '0';
            ebr.part_fit   = fitChar;
            ebr.part_start = startByte;
            ebr.part_s     = -1;
            ebr.part_next  = -1;
            std::memset(ebr.part_name, 0, 16);

            disk.seekp(startByte, std::ios::beg);
            disk.write(reinterpret_cast<char*>(&ebr), sizeof(EBR));
        }

        // Escribir MBR actualizado
        disk.seekp(0, std::ios::beg);
        disk.write(reinterpret_cast<char*>(&mbr), sizeof(MBR));
        disk.close();

        return "OK: Partición '" + name + "' creada exitosamente | Inicio: " +
               std::to_string(startByte) + " | Tamaño: " + std::to_string(sizeBytes) + " bytes";
    }

    // -----------------------------------------------
    // Crear partición lógica dentro de la extendida
    // -----------------------------------------------
    static std::string createLogical(std::fstream& disk, MBR& mbr,
                                     const std::string& path,
                                     long long sizeBytes, char fitChar,
                                     const std::string& name) {
        // Buscar partición extendida
        int extStart = -1, extSize = -1;
        for(int i = 0; i < 4; i++) {
            if(mbr.mbr_partitions[i].part_type == 'E' &&
               mbr.mbr_partitions[i].part_start != -1) {
                extStart = mbr.mbr_partitions[i].part_start;
                extSize  = mbr.mbr_partitions[i].part_s;
                break;
            }
        }
        if(extStart == -1)
            return "Error: No existe una partición extendida. Créala primero con -type=E";

        // Recorrer lista de EBRs
        EBR ebr;
        int currentPos  = extStart;
        int lastEBRPos  = -1;
        int usedSpace   = 0;

        while(true) {
            disk.seekg(currentPos, std::ios::beg);
            disk.read(reinterpret_cast<char*>(&ebr), sizeof(EBR));

            // Verificar nombre duplicado
            if(std::string(ebr.part_name) == name)
                return "Error: Ya existe una partición lógica con ese nombre: " + name;

            lastEBRPos = currentPos;

            if(ebr.part_next == -1) break;
            currentPos = ebr.part_next;
        }

        // Calcular dónde escribir el nuevo EBR
        int newEBRPos;
        if(ebr.part_s == -1) {
            // Primer EBR (vacío), usarlo directamente
            newEBRPos = extStart;
        } else {
            // Ir después del último EBR + su partición
            newEBRPos = lastEBRPos + sizeof(EBR) + ebr.part_s;
        }

        // Verificar que cabe dentro de la extendida
        if(newEBRPos + (int)sizeof(EBR) + (int)sizeBytes > extStart + extSize)
            return "Error: No hay espacio suficiente en la partición extendida";

        // Crear nuevo EBR
        EBR newEBR;
        newEBR.part_mount = '0';
        newEBR.part_fit   = fitChar;
        newEBR.part_start = newEBRPos + sizeof(EBR);
        newEBR.part_s     = (int)sizeBytes;
        newEBR.part_next  = -1;
        std::strncpy(newEBR.part_name, name.c_str(), 15);
        newEBR.part_name[15] = '\0';

        // Actualizar el EBR anterior para que apunte al nuevo
        if(ebr.part_s != -1) {
            ebr.part_next = newEBRPos;
            disk.seekp(lastEBRPos, std::ios::beg);
            disk.write(reinterpret_cast<char*>(&ebr), sizeof(EBR));
        }

        // Escribir nuevo EBR
        disk.seekp(newEBRPos, std::ios::beg);
        disk.write(reinterpret_cast<char*>(&newEBR), sizeof(EBR));
        disk.close();

        return "OK: Partición lógica '" + name + "' creada | Inicio: " +
               std::to_string(newEBR.part_start) + " | Tamaño: " +
               std::to_string(sizeBytes) + " bytes";
    }

    // -----------------------------------------------
    // Encontrar espacio libre en el disco
    // Soporta FF (First Fit), BF (Best Fit), WF (Worst Fit)
    // -----------------------------------------------
    static int findFreeSpace(MBR& mbr, int diskSize, long long neededSize, char fit) {
        // Construir lista de espacios ocupados
        std::vector<std::pair<int,int>> occupied; // {start, end}
        occupied.push_back({0, (int)sizeof(MBR)}); // El MBR siempre ocupa el inicio

        for(int i = 0; i < 4; i++) {
            if(mbr.mbr_partitions[i].part_start != -1) {
                int s = mbr.mbr_partitions[i].part_start;
                int e = s + mbr.mbr_partitions[i].part_s;
                occupied.push_back({s, e});
            }
        }

        // Ordenar por inicio
        std::sort(occupied.begin(), occupied.end());

        // Construir lista de espacios libres
        std::vector<std::pair<int,int>> freeSpaces; // {start, size}
        int prev = 0;
        for(const auto& occ : occupied) {
            if(occ.first > prev) {
                freeSpaces.push_back({prev, occ.first - prev});
            }
            prev = occ.second;
        }
        if(prev < diskSize) {
            freeSpaces.push_back({prev, diskSize - prev});
        }

        // Filtrar espacios que caben
        std::vector<std::pair<int,int>> candidates;
        for(const auto& fs : freeSpaces) {
            if(fs.second >= neededSize) candidates.push_back(fs);
        }
        if(candidates.empty()) return -1;

        if(fit == 'F') {
            // First Fit: el primero que cabe
            return candidates[0].first;
        } else if(fit == 'B') {
            // Best Fit: el más pequeño que cabe
            auto best = candidates[0];
            for(const auto& c : candidates)
                if(c.second < best.second) best = c;
            return best.first;
        } else {
            // Worst Fit: el más grande
            auto worst = candidates[0];
            for(const auto& c : candidates)
                if(c.second > worst.second) worst = c;
            return worst.first;
        }
    }
};

#endif // FDISK_H
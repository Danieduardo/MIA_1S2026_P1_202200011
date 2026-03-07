#ifndef MOUNT_H
#define MOUNT_H

#include <string>
#include <vector>
#include <fstream>
#include "../structs/Structs.h"
#include "../utils/Utils.h"
#include "../utils/MountedPartitions.h"

class Mount {
public:
    static std::string execute(const std::vector<std::pair<std::string,std::string>>& params) {
        std::string path = "";
        std::string name = "";

        for(const auto& p : params) {
            std::string key = toLower(p.first);
            if(key == "path")      path = p.second;
            else if(key == "name") name = p.second;
            else return "Error: Parámetro no reconocido -> " + p.first;
        }

        if(path.empty()) return "Error: -path es obligatorio";
        if(name.empty()) return "Error: -name es obligatorio";

        // Abrir disco
        std::fstream disk(path, std::ios::binary | std::ios::in | std::ios::out);
        if(!disk.is_open()) return "Error: No se pudo abrir el disco: " + path;

        // Leer MBR
        MBR mbr;
        disk.seekg(0, std::ios::beg);
        disk.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));

        // Buscar la partición por nombre (solo primarias)
        int partIdx = -1;
        for(int i = 0; i < 4; i++) {
            if(mbr.mbr_partitions[i].part_start != -1 &&
               mbr.mbr_partitions[i].part_type == 'P' &&
               std::string(mbr.mbr_partitions[i].part_name) == name) {
                partIdx = i;
                break;
            }
        }

        if(partIdx == -1)
            return "Error: No se encontró la partición primaria: " + name;

        // Verificar que no esté ya montada
        if(MountedPartitions::findByPathAndName(path, name) != nullptr)
            return "Error: La partición ya está montada";

        // Generar ID
        std::string id = MountedPartitions::getNextID(path);

        // Contar cuántas particiones del mismo disco están montadas
        int correlativo = 0;
        for(const auto& mp : MountedPartitions::mounted) {
            if(mp.path == path) correlativo++;
        }
        correlativo++;

        // Actualizar la partición en el MBR
        mbr.mbr_partitions[partIdx].part_status = '1';
        mbr.mbr_partitions[partIdx].part_correlative = correlativo;
        std::strncpy(mbr.mbr_partitions[partIdx].part_id, id.c_str(), 3);
        mbr.mbr_partitions[partIdx].part_id[3] = '\0';

        // Escribir MBR actualizado
        disk.seekp(0, std::ios::beg);
        disk.write(reinterpret_cast<char*>(&mbr), sizeof(MBR));
        disk.close();

        // Agregar a la lista en RAM
        MountedPartition mp;
        mp.id    = id;
        mp.path  = path;
        mp.name  = name;
        mp.start = mbr.mbr_partitions[partIdx].part_start;
        mp.size  = mbr.mbr_partitions[partIdx].part_s;
        MountedPartitions::mounted.push_back(mp);

        return "OK: Partición '" + name + "' montada con ID: " + id;
    }
};

// =============================================
// MOUNTED - Mostrar particiones montadas
// =============================================
class Mounted {
public:
    static std::string execute() {
        if(MountedPartitions::mounted.empty())
            return "No hay particiones montadas actualmente";

        std::string result = "Particiones montadas:\n";
        result += "--------------------------------\n";
        for(const auto& mp : MountedPartitions::mounted) {
            result += "ID: " + mp.id +
                      " | Disco: " + mp.path +
                      " | Partición: " + mp.name + "\n";
        }
        return result;
    }
};

#endif // MOUNT_H
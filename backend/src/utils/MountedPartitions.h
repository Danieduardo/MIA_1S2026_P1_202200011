#ifndef MOUNTEDPARTITIONS_H
#define MOUNTEDPARTITIONS_H

#include <string>
#include <vector>
#include <map>

// Información de una partición montada en RAM
struct MountedPartition {
    std::string id;       // Ej: 111A
    std::string path;     // Ruta del disco
    std::string name;     // Nombre de la partición
    int         start;    // Byte de inicio en el disco
    int         size;     // Tamaño en bytes
};

class MountedPartitions {
public:
    // Lista global de particiones montadas (en RAM)
    static std::vector<MountedPartition> mounted;

    // Carnet: últimos 2 dígitos = "11"
    static const std::string CARNET_SUFFIX;

    static std::string getNextID(const std::string& diskPath) {
        // Buscar si ya hay particiones de este disco montadas
        char letter = 'A';
        int  number = 1;

        // Buscar la letra actual según discos ya montados
        std::map<std::string, char> diskLetters;
        for(const auto& mp : mounted) {
            if(diskLetters.find(mp.path) == diskLetters.end()) {
                diskLetters[mp.path] = letter++;
            }
        }

        // Ver si este disco ya tiene letra
        if(diskLetters.find(diskPath) != diskLetters.end()) {
            // Contar particiones de este disco
            char myLetter = diskLetters[diskPath];
            int  count    = 0;
            for(const auto& mp : mounted) {
                if(mp.path == diskPath) count++;
            }
            return CARNET_SUFFIX + std::to_string(count + 1) + myLetter;
        } else {
            // Nuevo disco, nueva letra
            char newLetter = 'A';
            // La letra es la siguiente a todas las ya usadas
            std::vector<char> usedLetters;
            for(const auto& dl : diskLetters) usedLetters.push_back(dl.second);
            if(!usedLetters.empty()) {
                std::sort(usedLetters.begin(), usedLetters.end());
                newLetter = usedLetters.back() + 1;
            }
            return CARNET_SUFFIX + "1" + newLetter;
        }
    }

    static MountedPartition* findById(const std::string& id) {
        for(auto& mp : mounted) {
            if(mp.id == id) return &mp;
        }
        return nullptr;
    }

    static MountedPartition* findByPathAndName(const std::string& path,
                                                const std::string& name) {
        for(auto& mp : mounted) {
            if(mp.path == path && mp.name == name) return &mp;
        }
        return nullptr;
    }
};

// Definiciones estáticas
std::vector<MountedPartition> MountedPartitions::mounted;
const std::string MountedPartitions::CARNET_SUFFIX = "11";

#endif // MOUNTEDPARTITIONS_H
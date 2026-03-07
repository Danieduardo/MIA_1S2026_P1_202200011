#ifndef LOGIN_H
#define LOGIN_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "../structs/Structs.h"
#include "../utils/Utils.h"
#include "../utils/MountedPartitions.h"
#include "../utils/Session.h"

class Login {
public:
    static std::string execute(
        const std::vector<std::pair<std::string,std::string>>& params)
    {
        std::string user = "";
        std::string pass = "";
        std::string id   = "";

        for(const auto& p : params) {
            std::string key = toLower(p.first);
            // user y pass distinguen mayúsculas
            if(key == "user")      user = p.second;
            else if(key == "pass") pass = p.second;
            else if(key == "id")   id   = p.second;
            else return "Error: Parámetro no reconocido -> " + p.first;
        }

        if(user.empty()) return "Error: -user es obligatorio";
        if(pass.empty()) return "Error: -pass es obligatorio";
        if(id.empty())   return "Error: -id es obligatorio";

        // Verificar que no haya sesión activa
        if(currentSession.active)
            return "Error: Ya hay una sesión activa. Ejecute logout primero";

        // Buscar partición montada
        MountedPartition* mp = MountedPartitions::findById(id);
        if(mp == nullptr)
            return "Error: No existe partición montada con ID: " + id;

        // Leer users.txt desde el disco
        std::string usersContent = readUsersFile(mp);
        if(usersContent.empty())
            return "Error: No se pudo leer users.txt";

        // Buscar usuario en users.txt
        // Formato línea usuario: UID,U,grupo,usuario,contraseña
        std::istringstream ss(usersContent);
        std::string line;
        bool found = false;
        int  foundUID = -1;
        int  foundGID = -1;

        while(std::getline(ss, line)) {
            if(line.empty()) continue;
            std::vector<std::string> parts = splitCSV(line);
            if(parts.size() < 5) continue;
            if(trim(parts[1]) != "U") continue;

            std::string fileUser = trim(parts[3]);
            std::string filePass = trim(parts[4]);
            std::string statusStr= trim(parts[0]);

            // Verificar que no esté eliminado (id != 0)
            if(statusStr == "0") continue;

            // Comparar usuario y contraseña (case sensitive)
            if(fileUser == user && filePass == pass) {
                found    = true;
                foundUID = std::stoi(statusStr);
                // Buscar GID del grupo
                foundGID = getGID(usersContent, trim(parts[2]));
                break;
            }
        }

        if(!found)
            return "Error: Usuario o contraseña incorrectos";

        // Iniciar sesión
        currentSession.active   = true;
        currentSession.username = user;
        currentSession.id       = id;
        currentSession.uid      = foundUID;
        currentSession.gid      = foundGID;

        return "OK: Sesión iniciada como '" + user + "' en partición " + id;
    }

private:
    static std::string readUsersFile(MountedPartition* mp) {
        std::fstream disk(mp->path, std::ios::binary | std::ios::in);
        if(!disk.is_open()) return "";

        // Leer SuperBloque
        SuperBloque sb;
        disk.seekg(mp->start, std::ios::beg);
        disk.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

        // Leer inodo 0 (raíz)
        Inode rootInode;
        disk.seekg(sb.s_inode_start, std::ios::beg);
        disk.read(reinterpret_cast<char*>(&rootInode), sizeof(Inode));

        // Leer bloque carpeta raíz
        FolderBlock rootBlock;
        disk.seekg(sb.s_block_start, std::ios::beg);
        disk.read(reinterpret_cast<char*>(&rootBlock), sizeof(FolderBlock));

        // Buscar users.txt en la raíz
        int usersInodeNum = -1;
        for(int i = 0; i < 4; i++) {
            if(std::string(rootBlock.b_content[i].b_name) == "users.txt") {
                usersInodeNum = rootBlock.b_content[i].b_inodo;
                break;
            }
        }
        if(usersInodeNum == -1) { disk.close(); return ""; }

        // Leer inodo de users.txt
        Inode usersInode;
        disk.seekg(sb.s_inode_start + usersInodeNum * sizeof(Inode), std::ios::beg);
        disk.read(reinterpret_cast<char*>(&usersInode), sizeof(Inode));

        // Leer bloques de contenido
        std::string content = "";
        for(int i = 0; i < 12; i++) {
            if(usersInode.i_block[i] == -1) break;
            FileBlock fb;
            disk.seekg(sb.s_block_start + usersInode.i_block[i] * 64, std::ios::beg);
            disk.read(reinterpret_cast<char*>(&fb), sizeof(FileBlock));
            content += std::string(fb.b_content, 64);
        }

        disk.close();
        // Limpiar nulls del final
        content.erase(content.find_last_not_of('\0') + 1);
        return content;
    }

    static std::vector<std::string> splitCSV(const std::string& line) {
        std::vector<std::string> result;
        std::stringstream ss(line);
        std::string token;
        while(std::getline(ss, token, ',')) result.push_back(token);
        return result;
    }

    static int getGID(const std::string& content, const std::string& groupName) {
        std::istringstream ss(content);
        std::string line;
        while(std::getline(ss, line)) {
            auto parts = splitCSV(line);
            if(parts.size() < 3) continue;
            if(trim(parts[1]) == "G" && trim(parts[2]) == groupName)
                return std::stoi(trim(parts[0]));
        }
        return -1;
    }
};

// =============================================
// LOGOUT
// =============================================
class Logout {
public:
    static std::string execute() {
        if(!currentSession.active)
            return "Error: No hay sesión activa";

        std::string user = currentSession.username;
        currentSession.clear();
        return "OK: Sesión cerrada para usuario '" + user + "'";
    }
};

#endif // LOGIN_H
#ifndef MKFS_H
#define MKFS_H

#include <string>
#include <vector>
#include <fstream>
#include <cstring>
#include <cmath>
#include "../structs/Structs.h"
#include "../utils/Utils.h"
#include "../utils/MountedPartitions.h"

class MkFs {
public:
    static std::string execute(const std::vector<std::pair<std::string,std::string>>& params) {
        std::string id   = "";
        std::string type = "full";

        for(const auto& p : params) {
            std::string key = toLower(p.first);
            if(key == "id")        id   = p.second;
            else if(key == "type") type = toLower(p.second);
            else return "Error: Parámetro no reconocido -> " + p.first;
        }

        if(id.empty()) return "Error: -id es obligatorio";
        if(type != "full") return "Error: -type solo acepta 'full'";

        // Buscar partición montada
        MountedPartition* mp = MountedPartitions::findById(id);
        if(mp == nullptr)
            return "Error: No existe partición montada con ID: " + id;

        // Abrir disco
        std::fstream disk(mp->path, std::ios::binary | std::ios::in | std::ios::out);
        if(!disk.is_open())
            return "Error: No se pudo abrir el disco: " + mp->path;

        int partStart = mp->start;
        int partSize  = mp->size;

        // -----------------------------------------------
        // Calcular número de inodos y bloques
        // tamaño = sizeof(SB) + n + 3n + n*sizeof(Inode) + 3n*sizeof(Block)
        // Despejando n:
        // n = (partSize - sizeof(SB)) / (1 + 3 + sizeof(Inode) + 3*64)
        // -----------------------------------------------
        int sbSize    = sizeof(SuperBloque);
        int inodeSize = sizeof(Inode);
        int blockSize = 64;

        double n = (double)(partSize - sbSize) /
                   (1 + 3 + inodeSize + 3 * blockSize);
        int numStructures = (int)floor(n);

        if(numStructures <= 0) {
            disk.close();
            return "Error: La partición es demasiado pequeña para EXT2";
        }

        int numInodes = numStructures;
        int numBlocks = numStructures * 3;

        // -----------------------------------------------
        // Calcular posiciones dentro de la partición
        // -----------------------------------------------
        int bmInodeStart = partStart + sbSize;
        int bmBlockStart = bmInodeStart + numInodes;
        int inodeStart   = bmBlockStart + numBlocks;
        int blockStart   = inodeStart + numInodes * inodeSize;

        // -----------------------------------------------
        // Llenar bitmaps con ceros (todo libre)
        // -----------------------------------------------
        char zero = '0';
        disk.seekp(bmInodeStart, std::ios::beg);
        for(int i = 0; i < numInodes; i++) disk.write(&zero, 1);

        disk.seekp(bmBlockStart, std::ios::beg);
        for(int i = 0; i < numBlocks; i++) disk.write(&zero, 1);

        // -----------------------------------------------
        // Crear SuperBloque
        // -----------------------------------------------
        SuperBloque sb;
        sb.s_filesystem_type   = 2;
        sb.s_inodes_count      = numInodes;
        sb.s_blocks_count      = numBlocks;
        sb.s_free_inodes_count = numInodes;
        sb.s_free_blocks_count = numBlocks;
        sb.s_mtime             = time(nullptr);
        sb.s_umtime            = 0;
        sb.s_mnt_count         = 1;
        sb.s_magic             = 0xEF53;
        sb.s_inode_s           = inodeSize;
        sb.s_block_s           = blockSize;
        sb.s_firts_ino         = inodeStart;
        sb.s_first_blo         = blockStart;
        sb.s_bm_inode_start    = bmInodeStart;
        sb.s_bm_block_start    = bmBlockStart;
        sb.s_inode_start       = inodeStart;
        sb.s_block_start       = blockStart;

        disk.seekp(partStart, std::ios::beg);
        disk.write(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

        // -----------------------------------------------
        // Crear inodo raíz (inodo 0) -> carpeta "/"
        // -----------------------------------------------
        Inode rootInode;
        rootInode.i_uid    = 1;
        rootInode.i_gid    = 1;
        rootInode.i_s      = 0;
        rootInode.i_atime  = time(nullptr);
        rootInode.i_ctime  = time(nullptr);
        rootInode.i_mtime  = time(nullptr);
        rootInode.i_type   = '0'; // carpeta
        rootInode.i_perm[0]= '7';
        rootInode.i_perm[1]= '7';
        rootInode.i_perm[2]= '7';
        for(int i = 0; i < 15; i++) rootInode.i_block[i] = -1;
        rootInode.i_block[0] = 0; // apunta al bloque 0

        // Escribir inodo 0
        disk.seekp(inodeStart, std::ios::beg);
        disk.write(reinterpret_cast<char*>(&rootInode), sizeof(Inode));

        // Marcar inodo 0 como usado en bitmap
        char one = '1';
        disk.seekp(bmInodeStart, std::ios::beg);
        disk.write(&one, 1);

        // -----------------------------------------------
        // Crear bloque carpeta raíz (bloque 0)
        // Contiene "." y ".." apuntando al inodo 0
        // -----------------------------------------------
        FolderBlock rootBlock;
        std::strncpy(rootBlock.b_content[0].b_name, ".", 11);
        rootBlock.b_content[0].b_inodo = 0;
        std::strncpy(rootBlock.b_content[1].b_name, "..", 11);
        rootBlock.b_content[1].b_inodo = 0;
        rootBlock.b_content[2].b_inodo = -1;
        rootBlock.b_content[3].b_inodo = -1;

        disk.seekp(blockStart, std::ios::beg);
        disk.write(reinterpret_cast<char*>(&rootBlock), sizeof(FolderBlock));

        // Marcar bloque 0 como usado
        disk.seekp(bmBlockStart, std::ios::beg);
        disk.write(&one, 1);

        // -----------------------------------------------
        // Crear inodo para users.txt (inodo 1)
        // -----------------------------------------------
        std::string usersContent = "1,G,root\n1,U,root,root,123\n";
        int contentSize = usersContent.size();

        Inode usersInode;
        usersInode.i_uid    = 1;
        usersInode.i_gid    = 1;
        usersInode.i_s      = contentSize;
        usersInode.i_atime  = time(nullptr);
        usersInode.i_ctime  = time(nullptr);
        usersInode.i_mtime  = time(nullptr);
        usersInode.i_type   = '1'; // archivo
        usersInode.i_perm[0]= '7';
        usersInode.i_perm[1]= '7';
        usersInode.i_perm[2]= '7';
        for(int i = 0; i < 15; i++) usersInode.i_block[i] = -1;
        usersInode.i_block[0] = 1; // apunta al bloque 1

        // Escribir inodo 1
        disk.seekp(inodeStart + inodeSize, std::ios::beg);
        disk.write(reinterpret_cast<char*>(&usersInode), sizeof(Inode));

        // Marcar inodo 1 como usado
        disk.seekp(bmInodeStart + 1, std::ios::beg);
        disk.write(&one, 1);

        // -----------------------------------------------
        // Crear bloque archivo para users.txt (bloque 1)
        // -----------------------------------------------
        FileBlock usersBlock;
        std::memset(usersBlock.b_content, 0, 64);
        std::strncpy(usersBlock.b_content, usersContent.c_str(), 63);

        disk.seekp(blockStart + blockSize, std::ios::beg);
        disk.write(reinterpret_cast<char*>(&usersBlock), sizeof(FileBlock));

        // Marcar bloque 1 como usado
        disk.seekp(bmBlockStart + 1, std::ios::beg);
        disk.write(&one, 1);

        // -----------------------------------------------
        // Agregar users.txt al bloque raíz
        // -----------------------------------------------
        std::strncpy(rootBlock.b_content[2].b_name, "users.txt", 11);
        rootBlock.b_content[2].b_inodo = 1;

        disk.seekp(blockStart, std::ios::beg);
        disk.write(reinterpret_cast<char*>(&rootBlock), sizeof(FolderBlock));

        // -----------------------------------------------
        // Actualizar SuperBloque: 2 inodos y 2 bloques usados
        // -----------------------------------------------
        sb.s_free_inodes_count -= 2;
        sb.s_free_blocks_count -= 2;
        sb.s_firts_ino          = inodeStart + 2 * inodeSize;
        sb.s_first_blo          = blockStart + 2 * blockSize;

        disk.seekp(partStart, std::ios::beg);
        disk.write(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

        disk.close();

        return "OK: Partición formateada como EXT2\n"
               "  Inodos totales:  " + std::to_string(numInodes) + "\n"
               "  Bloques totales: " + std::to_string(numBlocks)  + "\n"
               "  Archivo users.txt creado en la raíz";
    }
};

#endif // MKFS_H
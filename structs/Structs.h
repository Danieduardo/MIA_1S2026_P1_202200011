#ifndef STRUCTS_H
#define STRUCTS_H

#include <ctime>
#include <cstring>

// =============================================
// PARTITION
// =============================================
struct Partition {
    char part_status;
    char part_type;
    char part_fit;
    int  part_start;
    int  part_s;
    char part_name[16];
    int  part_correlative;
    char part_id[4];

    Partition() {
        part_status      = '0';
        part_type        = 'P';
        part_fit         = 'W';
        part_start       = -1;
        part_s           = -1;
        part_correlative = -1;
        memset(part_name, 0, 16);
        memset(part_id,   0, 4);
    }
};

// =============================================
// MBR
// =============================================
struct MBR {
    int       mbr_tamano;
    time_t    mbr_fecha_creacion;
    int       mbr_dsk_signature;
    char      dsk_fit;
    Partition mbr_partitions[4];

    MBR() {
        mbr_tamano         = 0;
        mbr_fecha_creacion = time(nullptr);
        mbr_dsk_signature  = 0;
        dsk_fit            = 'F';
    }
};

// =============================================
// EBR
// =============================================
struct EBR {
    char part_mount;
    char part_fit;
    int  part_start;
    int  part_s;
    int  part_next;
    char part_name[16];

    EBR() {
        part_mount = '0';
        part_fit   = 'W';
        part_start = -1;
        part_s     = -1;
        part_next  = -1;
        memset(part_name, 0, 16);
    }
};

// =============================================
// CONTENT (parte del bloque carpeta)
// =============================================
struct Content {
    char b_name[12];
    int  b_inodo;

    Content() {
        memset(b_name, 0, 12);
        b_inodo = -1;
    }
};

// =============================================
// BLOQUE CARPETA
// =============================================
struct FolderBlock {
    Content b_content[4];

    FolderBlock() {
        for(int i = 0; i < 4; i++) {
            memset(b_content[i].b_name, 0, 12);
            b_content[i].b_inodo = -1;
        }
    }
};

// =============================================
// BLOQUE ARCHIVO
// =============================================
struct FileBlock {
    char b_content[64];

    FileBlock() {
        memset(b_content, 0, 64);
    }
};

// =============================================
// BLOQUE APUNTADORES
// =============================================
struct PointerBlock {
    int b_pointers[16];

    PointerBlock() {
        for(int i = 0; i < 16; i++) b_pointers[i] = -1;
    }
};

// =============================================
// INODO
// (definido ANTES del SuperBloque para que
//  sizeof(Inode) funcione correctamente)
// =============================================
struct Inode {
    int    i_uid;
    int    i_gid;
    int    i_s;
    time_t i_atime;
    time_t i_ctime;
    time_t i_mtime;
    int    i_block[15];
    char   i_type;
    char   i_perm[3];

    Inode() {
        i_uid   = -1;
        i_gid   = -1;
        i_s     = 0;
        i_atime = time(nullptr);
        i_ctime = time(nullptr);
        i_mtime = time(nullptr);
        for(int i = 0; i < 15; i++) i_block[i] = -1;
        i_type    = '0';
        i_perm[0] = '6';
        i_perm[1] = '6';
        i_perm[2] = '4';
    }
};

// =============================================
// SUPERBLOQUE
// (definido DESPUÉS de Inode para poder usar
//  sizeof(Inode) sin problemas)
// =============================================
struct SuperBloque {
    int    s_filesystem_type;
    int    s_inodes_count;
    int    s_blocks_count;
    int    s_free_blocks_count;
    int    s_free_inodes_count;
    time_t s_mtime;
    time_t s_umtime;
    int    s_mnt_count;
    int    s_magic;
    int    s_inode_s;
    int    s_block_s;
    int    s_firts_ino;
    int    s_first_blo;
    int    s_bm_inode_start;
    int    s_bm_block_start;
    int    s_inode_start;
    int    s_block_start;

    SuperBloque() {
        s_filesystem_type   = 2;
        s_inodes_count      = 0;
        s_blocks_count      = 0;
        s_free_blocks_count = 0;
        s_free_inodes_count = 0;
        s_mtime             = time(nullptr);
        s_umtime            = 0;
        s_mnt_count         = 0;
        s_magic             = 0xEF53;
        s_inode_s           = sizeof(Inode);  // ✓ Ahora Inode ya existe
        s_block_s           = 64;
        s_firts_ino         = 0;
        s_first_blo         = 0;
        s_bm_inode_start    = 0;
        s_bm_block_start    = 0;
        s_inode_start       = 0;
        s_block_start       = 0;
    }
};

#endif // STRUCTS_H
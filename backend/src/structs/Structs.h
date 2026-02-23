#ifndef STRUCTS_H
#define STRUCTS_H

#include <ctime>

// =============================================
// PARTITION - Estructura de partición
// =============================================
struct Partition {
    char part_status;      // Montada o no
    char part_type;        // P = Primaria, E = Extendida
    char part_fit;         // B = Best, F = First, W = Worst
    int  part_start;       // Byte de inicio en el disco
    int  part_s;           // Tamaño en bytes
    char part_name[16];    // Nombre de la partición
    int  part_correlative; // Correlativo (-1 hasta montar)
    char part_id[4];       // ID generado al montar

    Partition() {
        part_status      = '0';
        part_type        = 'P';
        part_fit         = 'W';
        part_start       = -1;
        part_s           = -1;
        part_correlative = -1;
        for(int i = 0; i < 16; i++) part_name[i] = '\0';
        for(int i = 0; i < 4;  i++) part_id[i]   = '\0';
    }
};

// =============================================
// MBR - Master Boot Record
// =============================================
struct MBR {
    int       mbr_tamano;        // Tamaño total del disco en bytes
    time_t    mbr_fecha_creacion;// Fecha y hora de creación
    int       mbr_dsk_signature; // Número random único
    char      dsk_fit;           // B, F o W
    Partition mbr_partitions[4]; // 4 particiones

    MBR() {
        mbr_tamano         = 0;
        mbr_fecha_creacion = time(nullptr);
        mbr_dsk_signature  = 0;
        dsk_fit            = 'F';
    }
};

// =============================================
// EBR - Extended Boot Record
// =============================================
struct EBR {
    char part_mount; // Montada o no
    char part_fit;   // B, F o W
    int  part_start; // Byte de inicio
    int  part_s;     // Tamaño en bytes
    int  part_next;  // Byte del próximo EBR (-1 si no hay)
    char part_name[16];

    EBR() {
        part_mount = '0';
        part_fit   = 'W';
        part_start = -1;
        part_s     = -1;
        part_next  = -1;
        for(int i = 0; i < 16; i++) part_name[i] = '\0';
    }
};

// =============================================
// SUPERBLOQUE - Información del sistema EXT2
// =============================================
struct SuperBloque {
    int    s_filesystem_type;  // Tipo: 2 para EXT2
    int    s_inodes_count;     // Total de inodos
    int    s_blocks_count;     // Total de bloques
    int    s_free_blocks_count;// Bloques libres
    int    s_free_inodes_count;// Inodos libres
    time_t s_mtime;            // Último montaje
    time_t s_umtime;           // Último desmontaje
    int    s_mnt_count;        // Veces montado
    int    s_magic;            // 0xEF53
    int    s_inode_s;          // Tamaño del inodo
    int    s_block_s;          // Tamaño del bloque
    int    s_firts_ino;        // Primer inodo libre
    int    s_first_blo;        // Primer bloque libre
    int    s_bm_inode_start;   // Inicio bitmap inodos
    int    s_bm_block_start;   // Inicio bitmap bloques
    int    s_inode_start;      // Inicio tabla inodos
    int    s_block_start;      // Inicio tabla bloques

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
        s_inode_s           = sizeof(struct Inode);
        s_block_s           = 64;
        s_firts_ino         = 0;
        s_first_blo         = 0;
        s_bm_inode_start    = 0;
        s_bm_block_start    = 0;
        s_inode_start       = 0;
        s_block_start       = 0;
    }
};

// =============================================
// INODO - Index Node
// =============================================
struct Inode {
    int    i_uid;      // UID del propietario
    int    i_gid;      // GID del grupo
    int    i_s;        // Tamaño en bytes
    time_t i_atime;    // Último acceso
    time_t i_ctime;    // Creación
    time_t i_mtime;    // Última modificación
    int    i_block[15];// Apuntadores (12 directos + 3 indirectos)
    char   i_type;     // '1' = Archivo, '0' = Carpeta
    char   i_perm[3];  // Permisos UGO octal

    Inode() {
        i_uid   = -1;
        i_gid   = -1;
        i_s     = 0;
        i_atime = time(nullptr);
        i_ctime = time(nullptr);
        i_mtime = time(nullptr);
        for(int i = 0; i < 15; i++) i_block[i] = -1;
        i_type  = '0';
        i_perm[0] = '6';
        i_perm[1] = '6';
        i_perm[2] = '4';
    }
};

// =============================================
// BLOQUE CARPETA
// =============================================
struct Content {
    char b_name[12]; // Nombre carpeta/archivo
    int  b_inodo;    // Apuntador al inodo

    Content() {
        for(int i = 0; i < 12; i++) b_name[i] = '\0';
        b_inodo = -1;
    }
};

struct FolderBlock {
    Content b_content[4]; // 4 entradas por bloque carpeta
    // Tamaño: 4*(12+4) = 64 bytes ✓
};

// =============================================
// BLOQUE ARCHIVO
// =============================================
struct FileBlock {
    char b_content[64]; // Contenido del archivo (64 chars)
    // Tamaño: 64 bytes ✓
};

// =============================================
// BLOQUE APUNTADORES
// =============================================
struct PointerBlock {
    int b_pointers[16]; // 16 apuntadores
    // Tamaño: 16*4 = 64 bytes ✓

    PointerBlock() {
        for(int i = 0; i < 16; i++) b_pointers[i] = -1;
    }
};

#endif // STRUCTS_H
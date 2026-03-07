#ifndef SESSION_H
#define SESSION_H

#include <string>

// Sesión activa global
struct Session {
    bool        active   = false;
    std::string username = "";
    std::string id       = "";   // ID de partición montada
    int         uid      = -1;
    int         gid      = -1;

    void clear() {
        active   = false;
        username = "";
        id       = "";
        uid      = -1;
        gid      = -1;
    }
};

// Instancia global
static Session currentSession;

#endif // SESSION_H
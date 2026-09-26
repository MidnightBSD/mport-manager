#ifndef MOCK_MPORT_H
#define MOCK_MPORT_H

#include <stdbool.h>

#if __has_include(<mport.h>) && defined(__MidnightBSD__)
#include <mport.h>
#else

#define MPORT_OK 0
#define MPORT_ERR_WARN 1
#define MPORT_ERR_FATAL -1
#define MPORT_VNORMAL 0
#define MPORT_EXPLICIT 0
#define MPORT_AUTOMATIC 1
#define MPORT_TYPE_SYSTEM 1
#define MPORT_ACTION_DELETE 1
#define MPORT_LOCKED 1

typedef enum { MPORT_EXPLICIT_ENUM = 0 } mportAutomatic;

typedef struct {
    void (*msg_cb)(const char *);
    int (*confirm_cb)(const char *, const char *, const char *, int);
    int (*select_cb)(const char *, void **, int);
    void (*progress_init_cb)(const char *);
    void (*progress_step_cb)(int, int, const char *);
    void (*progress_free_cb)(void);
    bool force;
} mportInstance;

typedef struct {
    char *pkgname;
    char *version;
    char *comment;
    char *license;
    int type;
} mportIndexEntry;

typedef struct {
    char *name;
    char *version;
    char *os_release;
    int flatsize;
    int action;
} mportPackageMeta;

typedef struct {
    unsigned int pkg_installed;
    unsigned int pkg_available;
    long long pkg_installed_size;
} mportStats;

typedef struct {
    char *d_pkgname;
    char *d_version;
} mportDependsEntry;

typedef struct {
    char country[32];
    char url[256];
} mportMirrorEntry;

mportInstance *mport_instance_new(void);
int mport_instance_init(mportInstance *, const char *, const char *, bool, int);
void mport_instance_free(mportInstance *);
const char *mport_err_string(void);
int mport_err_code(void);
void mport_set_msg_cb(mportInstance *, void (*)(const char *));
void mport_set_confirm_cb(mportInstance *, int (*)(const char *, const char *, const char *, int));
void mport_set_select_cb(mportInstance *, int (*)(const char *, void **, int));
void mport_set_progress_init_cb(mportInstance *, void (*)(const char *));
void mport_set_progress_step_cb(mportInstance *, void (*)(int, int, const char *));
void mport_set_progress_free_cb(mportInstance *, void (*)(void));
int mport_index_load(mportInstance *);
int mport_index_lookup_pkgname(mportInstance *, const char *, mportIndexEntry ***);
void mport_index_entry_free_vec(mportIndexEntry **);
int mport_version_cmp(const char *, const char *);
int mport_stats(mportInstance *, mportStats **);
void mport_stats_free(mportStats *);
int mport_upgrade(mportInstance *);
int mport_index_search_term(mportInstance *, mportIndexEntry ***, char *);
int mport_index_list(mportInstance *, mportIndexEntry ***);
int mport_pkgmeta_list(mportInstance *, mportPackageMeta ***);
void mport_pkgmeta_vec_free(mportPackageMeta **);
int mport_lock_islocked(mportPackageMeta *);
char *mport_get_osrelease(mportInstance *);
int mport_pkgmeta_search_master(mportInstance *, mportPackageMeta ***, const char *, ...);
int mport_lock_lock(mportInstance *, mportPackageMeta *);
int mport_lock_unlock(mportInstance *, mportPackageMeta *);
int mport_index_depends_list(mportInstance *, const char *, const char *, mportDependsEntry ***);
void mport_index_depends_free_vec(mportDependsEntry **);
int mport_install(mportInstance *, const char *, const char *, const char *, mportAutomatic);
int mport_delete_primative(mportInstance *, mportPackageMeta *, int);
int mport_autoremove(mportInstance *);
int mport_clean_database(mportInstance *);
int mport_clean_oldpackages(mportInstance *);
int mport_clean_oldmtree(mportInstance *);
int mport_clean_tempfiles(mportInstance *);
int mport_verify_package(mportInstance *, mportPackageMeta *);
int mport_index_mirror_list(mportInstance *, mportMirrorEntry ***);
void mport_index_mirror_entry_free_vec(mportMirrorEntry **);
long ping(const char *);
int mport_setting_set(mportInstance *, const char *, const char *);
int mport_import(mportInstance *, const char *);
int mport_export(mportInstance *, const char *);
char *mport_audit(mportInstance *, const char *, bool);

#endif
#endif

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libutil.h"
#include "mport.h"

int humanize_number(char *buf, size_t len, int64_t bytes, const char *suffix, int scale, int flags) {
    if (buf && len > 0) snprintf(buf, len, "%lld %s", (long long)bytes, suffix ? suffix : "");
    return 0;
}

mportInstance *mport_instance_new(void) { return calloc(1, sizeof(mportInstance)); }
int mport_instance_init(mportInstance *m, const char *a, const char *b, bool c, int d) { return MPORT_OK; }
void mport_instance_free(mportInstance *m) { free(m); }
const char *mport_err_string(void) { return ""; }
int mport_err_code(void) { return 0; }
void mport_set_msg_cb(mportInstance *m, void (*cb)(const char *)) { if (m) m->msg_cb = cb; }
void mport_set_confirm_cb(mportInstance *m, int (*cb)(const char *, const char *, const char *, int)) { if (m) m->confirm_cb = cb; }
void mport_set_select_cb(mportInstance *m, int (*cb)(const char *, void **, int)) { if (m) m->select_cb = cb; }
void mport_set_progress_init_cb(mportInstance *m, void (*cb)(const char *)) { if (m) m->progress_init_cb = cb; }
void mport_set_progress_step_cb(mportInstance *m, void (*cb)(int, int, const char *)) { if (m) m->progress_step_cb = cb; }
void mport_set_progress_free_cb(mportInstance *m, void (*cb)(void)) { if (m) m->progress_free_cb = cb; }
int mport_index_load(mportInstance *m) { return MPORT_OK; }
int mport_index_lookup_pkgname(mportInstance *m, const char *p, mportIndexEntry ***e) { *e = NULL; return MPORT_OK; }
void mport_index_entry_free_vec(mportIndexEntry **e) {}
int mport_version_cmp(const char *a, const char *b) { return 0; }
int mport_stats(mportInstance *m, mportStats **s) { *s = calloc(1, sizeof(mportStats)); return MPORT_OK; }
void mport_stats_free(mportStats *s) { free(s); }
int mport_upgrade(mportInstance *m) { return MPORT_OK; }
int mport_index_search_term(mportInstance *m, mportIndexEntry ***e, char *s) { *e = NULL; return MPORT_OK; }
int mport_index_list(mportInstance *m, mportIndexEntry ***e) { *e = NULL; return MPORT_OK; }
int mport_pkgmeta_list(mportInstance *m, mportPackageMeta ***p) { *p = NULL; return MPORT_OK; }
void mport_pkgmeta_vec_free(mportPackageMeta **p) {}
int mport_lock_islocked(mportPackageMeta *p) { return 0; }
char *mport_get_osrelease(mportInstance *m) { return NULL; }
int mport_pkgmeta_search_master(mportInstance *m, mportPackageMeta ***p, const char *q, ...) { *p = NULL; return MPORT_OK; }
int mport_lock_lock(mportInstance *m, mportPackageMeta *p) { return MPORT_OK; }
int mport_lock_unlock(mportInstance *m, mportPackageMeta *p) { return MPORT_OK; }
int mport_index_depends_list(mportInstance *m, const char *p, const char *v, mportDependsEntry ***d) { *d = NULL; return MPORT_OK; }
void mport_index_depends_free_vec(mportDependsEntry **d) {}
int mport_install(mportInstance *m, const char *p, const char *v, const char *i, mportAutomatic a) { return MPORT_OK; }
int mport_delete_primative(mportInstance *m, mportPackageMeta *p, int f) { return MPORT_OK; }
int mport_autoremove(mportInstance *m) { return MPORT_OK; }
int mport_clean_database(mportInstance *m) { return MPORT_OK; }
int mport_clean_oldpackages(mportInstance *m) { return MPORT_OK; }
int mport_clean_oldmtree(mportInstance *m) { return MPORT_OK; }
int mport_clean_tempfiles(mportInstance *m) { return MPORT_OK; }
int mport_verify_package(mportInstance *m, mportPackageMeta *p) { return MPORT_OK; }
int mport_index_mirror_list(mportInstance *m, mportMirrorEntry ***me) { *me = NULL; return MPORT_OK; }
void mport_index_mirror_entry_free_vec(mportMirrorEntry **me) {}
long ping(const char *host) { return 10; }
int mport_setting_set(mportInstance *m, const char *k, const char *v) { return MPORT_OK; }
int mport_import(mportInstance *m, const char *f) { return MPORT_OK; }
int mport_export(mportInstance *m, const char *f) { return MPORT_OK; }
char *mport_audit(mportInstance *m, const char *p, bool f) { return NULL; }

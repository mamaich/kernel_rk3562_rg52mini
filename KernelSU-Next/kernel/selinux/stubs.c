#include "selinux.h"
u32 ksu_file_sid;

void setup_selinux(const char *c, struct cred *cr) {}
void setenforce(bool e) {}
bool getenforce(void) { return false; }
void cache_sid(void) {}
bool is_task_ksu_domain(const struct cred *cr) { return false; }
bool is_ksu_domain(void) { return false; }
bool is_zygote(const struct cred *cr) { return false; }
bool is_init(const struct cred *cr) { return false; }
void apply_kernelsu_rules(void) {}
int handle_sepolicy(void __user *u, u64 l) { return -1; }
void setup_ksu_cred(void) {}
void escape_to_root_for_adb_root(void) {}
struct inode_security_struct *selinux_inode(const struct inode *i) { return NULL; }

void ksu_selinux_hide_init(void) {}
void ksu_selinux_hide_exit(void) {}
void ksu_selinux_hide_handle_post_fs_data(void) {}
void ksu_selinux_hide_drop_backup_if_unused(void) {}
void ksu_selinux_hide_handle_second_stage(void) {}

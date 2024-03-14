#define NTY_FILE_ADD 1
#define NTY_FILE_CHG 2
#define NTY_FILE_RM 3

int inotify_handler_init( mtp_ctx * ctx );
int inotify_handler_deinit( mtp_ctx * ctx );

int inotify_handler_addwatch( mtp_ctx * ctx, char * path );
int inotify_handler_rmwatch( mtp_ctx * ctx, int wd );
int inotify_handler_filechange(int type, char *path);

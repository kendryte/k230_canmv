import os

S_IFMT = 0o170000
S_IFDIR = 0o040000
S_IFREG = 0o100000

def sizeof_fmt(num, suffix='B'):
    for unit in ['', 'K', 'M', 'G', 'T', 'P', 'E', 'Z']:
        if abs(num) < 1024.0:
            return "%3.1f%s%s" % (num, unit, suffix)
        num /= 1024.0
    return "%.1f%s%s" % (num, 'Yi', suffix)

root_files = os.listdir('/')
fs_info_list = []
for f in root_files:
    fs_path = '/' + f
    fs_stat = os.stat(fs_path)
    size = fs_stat[6]
    f_type = fs_stat[0] & S_IFMT
    if f_type == S_IFDIR:
        f_type = "DIR"
    elif f_type == S_IFREG:
        f_type = "FILE"
    else:
        f_type = "OTHER"
    info = "%s [%s] size=%s" % (
        fs_path, f_type,
        sizeof_fmt(size)
    )
    fs_info_list.append(info)
    print(info)

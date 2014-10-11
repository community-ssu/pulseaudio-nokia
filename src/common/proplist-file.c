#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pulse/proplist.h>
#include <pulse/xmalloc.h>
#include <pulsecore/log.h>
#include <inttypes.h>

#include "proplist-file.h"

int pa_nokia_proplist_to_file(const char *filename, pa_proplist *p)
{
  FILE *fp;
  char *s;

  fp = fopen(filename, "w+");
  if (fp)
  {
    s = pa_proplist_to_string(p);
    fputs(s, fp);
    fclose(fp);
    pa_xfree(s);
    return 0;
  }

  return -1;
}

pa_proplist *pa_nokia_proplist_from_file(const char *filename)
{
  pa_proplist *p = NULL;
  FILE *fp;
  char *buf;
  size_t bytes;
  struct stat stat_buf;

  if (!stat(filename, &stat_buf))
  {
    pa_log_debug("file size: %ld bytes", stat_buf.st_size);

    fp = fopen(filename, "r");
    if (fp)
    {
      buf = pa_xmalloc(stat_buf.st_size + 1);
      bytes = fread(buf, 1, stat_buf.st_size, fp);
      pa_log_debug("read %d bytes", bytes);
      fclose(fp);

      if (bytes == (size_t)stat_buf.st_size)
      {
        buf[bytes] = 0;
        p = pa_proplist_from_string(buf);
      }

      pa_xfree(buf);

      if (p)
        return p;
    }
  }

  return pa_proplist_new();
}

void pa_nokia_update_proplist_to_file(const char *filename,
                                      pa_update_mode_t mode,
                                      pa_proplist *update)
{
  pa_proplist *p;

  p = pa_nokia_proplist_from_file(filename);
  pa_proplist_update(p, mode, update);
  pa_nokia_proplist_to_file(filename, p);
  pa_proplist_free(p);
}

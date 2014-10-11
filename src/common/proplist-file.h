#ifndef PROPLISTFILE_H
#define PROPLISTFILE_H

int pa_nokia_proplist_to_file(const char *filename, pa_proplist *p);
pa_proplist *pa_nokia_proplist_from_file(const char *filename);
void pa_nokia_update_proplist_to_file(const char *filename,
                                      pa_update_mode_t mode,
                                      pa_proplist *update);
#endif // PROPLISTFILE_H

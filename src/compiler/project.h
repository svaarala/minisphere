#ifndef CELL__PROJECT_H__INCLUDED
#define CELL__PROJECT_H__INCLUDED

typedef struct project project_t;

project_t* project_new           (const path_t* path, const char* title);
project_t* project_open          (const path_t* path);
void       project_free          (project_t* proj);
void       project_set_api_level (project_t* proj, int level);
void       project_set_author    (project_t* proj, const char* name);
void       project_set_res       (project_t* proj, int width, int height);
void       project_set_summary   (project_t* proj, const char* summary);
void       project_set_title     (project_t* proj, const char* title);
void       project_save          (project_t* proj);

#endif // CELL__PROJECT_H__INCLUDED

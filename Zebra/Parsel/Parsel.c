//
//  Parsel.c
//  Zebra
//
//  Created by Wilson Styres on 11/30/18.
//  Copyright © 2018 Wilson Styres. All rights reserved.
//

#include "Parsel.h"

typedef char *multi_tok_t;

char *multi_tok(char *input, multi_tok_t *string, char *delimiter) {
    if (input != NULL)
        *string = input;
    
    if (*string == NULL)
        return *string;
    
    char *end = strstr(*string, delimiter);
    if (end == NULL) {
        char *temp = *string;
        *string = NULL;
        return temp;
    }
    
    char *temp = *string;
    
    *end = '\0';
    *string = end + strlen(delimiter);
    return temp;
}

multi_tok_t init() { return NULL; }

char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos){
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}

int isRepoSecure(char *repoURL) {
    FILE *file = fopen("/var/lib/zebra/sources.list", "r");
    if (file != NULL) {
        char line[256];
        
        char *url = strtok(repoURL, "_");
        
        while (fgets(line, sizeof(line), file) != NULL) {
            if (strcasestr(line, url) != NULL && line[8] == 's') {
                return 1;
            }
        }
        
        return 0;
    }
    else {
        return 0;
    }
}

void importRepoToDatabase(const char *path, sqlite3 *database, int repoID) {
    FILE *file = fopen(path, "r");
    char line[256];
    
    char *sql = "CREATE TABLE IF NOT EXISTS REPOS(ORIGIN STRING, DESCRIPTION STRING, BASEFILENAME STRING, BASEURL STRING, SECURE INTEGER, REPOID INTEGER);";
    sqlite3_exec(database, sql, NULL, 0, NULL);
    
    char repo[4][256];
    while (fgets(line, sizeof(line), file) != NULL) {
        char *info = strtok(line, "\n");
        
        multi_tok_t s = init();
        
        char *key = multi_tok(info, &s, ": ");
        
        if (strcmp(key, "Origin") == 0) {
            char *value = multi_tok(NULL, &s, ": ");
            strcpy(repo[0], value);
        }
        else if (strcmp(key, "Description") == 0) {
            char *value = multi_tok(NULL, &s, ": ");
            strcpy(repo[1], value);
        }
    }
    
    multi_tok_t t = init();
    char *fullfilename = basename((char *)path);
    char *baseFilename = multi_tok(fullfilename, &t, "_Release");
    strcpy(repo[2], baseFilename);
    
    char secureURL[128];
    strcpy(secureURL, baseFilename);
    int secure = isRepoSecure(secureURL);
    
    replace_char(baseFilename, '_', '/');
    strcpy(repo[3], baseFilename);
    
    sqlite3_stmt *insertStatement;
    char *insertQuery = "INSERT INTO REPOS(ORIGIN, DESCRIPTION, BASEFILENAME, BASEURL, SECURE, REPOID) VALUES(?, ?, ?, ?, ?, ?);";
    
    if (sqlite3_prepare_v2(database, insertQuery, -1, &insertStatement, 0) == SQLITE_OK) {
        sqlite3_bind_text(insertStatement, 1, repo[0], -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insertStatement, 2, repo[1], -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insertStatement, 3, repo[2], -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insertStatement, 4, repo[3], -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(insertStatement, 5, secure);
        sqlite3_bind_int(insertStatement, 6, repoID);
        sqlite3_step(insertStatement);
    }
    else {
        printf("sql error: %s", sqlite3_errmsg(database));
    }
    
    repo[0][0] = 0;
    repo[1][0] = 0;
    repo[2][0] = 0;
    repo[3][0] = 0;
    
    fclose(file);
}

void importPackagesToDatabase(const char *path, sqlite3 *database, int repoID) {
    FILE *file = fopen(path, "r");
    char line[256];
    
    char *sql = "CREATE TABLE IF NOT EXISTS PACKAGES(PACKAGE STRING, NAME STRING, VERSION STRING, DESC STRING, SECTION STRING, DEPICTION STRING, REPOID INTEGER);";
    sqlite3_exec(database, sql, NULL, 0, NULL);
    sqlite3_exec(database, "BEGIN TRANSACTION", NULL, NULL, NULL);
    
    char package[6][1024];
    while (fgets(line, sizeof(line), file)) {
        if (strcmp(line, "\n") != 0) {
            char *info = strtok(line, "\n");

            multi_tok_t s = init();
            
            char *key = multi_tok(info, &s, ": ");
            
            if (strcmp(key, "Package") == 0) {
                char *value = multi_tok(NULL, &s, ": ");
                strcpy(package[0], value);
            }
            else if (strcmp(key, "Name") == 0) {
                char *value = multi_tok(NULL, &s, ": ");
                strcpy(package[1], value);
            }
            else if (strcmp(key, "Version") == 0) {
                char *value = multi_tok(NULL, &s, ": ");
                strcpy(package[2], value);
            }
            else if (strcmp(key, "Description") == 0) {
                char *value = multi_tok(NULL, &s, ": ");
                strcpy(package[3], value);
            }
            else if (strcmp(key, "Section") == 0) {
                char *value = multi_tok(NULL, &s, ": ");
                strcpy(package[4], value);
            }
            else if (strcmp(key, "Depiction") == 0) {
                char *value = multi_tok(NULL, &s, ": ");
                strcpy(package[5], value);
            }
            
        }
        else {
            
            if (strcasestr(package[0], "saffron-jailbreak") == NULL && strcasestr(package[0], "gsc") == NULL && strcasestr(package[0], "cy+") == NULL) {
                if (package[1][0] == 0) {
                    strcpy(package[1], package[0]);
                }
                
                sqlite3_stmt *insertStatement;
                char *insertQuery = "INSERT INTO PACKAGES(PACKAGE, NAME, VERSION, DESC, SECTION, DEPICTION, REPOID) VALUES(?, ?, ?, ?, ?, ?, ?);";
                
                if (sqlite3_prepare_v2(database, insertQuery, -1, &insertStatement, 0) == SQLITE_OK) {
                    sqlite3_bind_text(insertStatement, 1, package[0], -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(insertStatement, 2, package[1], -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(insertStatement, 3, package[2], -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(insertStatement, 4, package[3], -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(insertStatement, 5, package[4], -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(insertStatement, 6, package[5], -1, SQLITE_TRANSIENT);
                    sqlite3_bind_int(insertStatement, 7, repoID);
                    sqlite3_step(insertStatement);
                }
                else {
                    printf("database error: %s", sqlite3_errmsg(database));
                }
                
                package[0][0] = 0;
                package[1][0] = 0;
                package[2][0] = 0;
                package[3][0] = 0;
                package[4][0] = 0;
                package[5][0] = 0;
            }
            else {
                continue;
            }
        }
    }
    
    fclose(file);
    sqlite3_exec(database, "COMMIT TRANSACTION", NULL, NULL, NULL);
}
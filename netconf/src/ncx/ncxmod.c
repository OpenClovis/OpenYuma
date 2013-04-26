/*
 * Copyright (c) 2008 - 2012, Andy Bierman, All Rights Reserved.
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.    
 */
/*  FILE: ncxmod.c

                
*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
10nov05      abb      begun

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <unistd.h>
#include <xmlstring.h>
#include <xmlreader.h>

#include "procdefs.h"
#include "help.h"
#include "log.h"
#include "ncx.h"
#include "ncxconst.h"
#include "ncxtypes.h"
#include "ncxmod.h"
#include "status.h"
#include "tstamp.h"
#include "xml_util.h"
#include "yangconst.h"
#include "yang_parse.h"


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/


/* Enumeration of the basic value type classifications */
typedef enum ncxmod_mode_t_ {
    NCXMOD_MODE_NONE,
    NCXMOD_MODE_YANG,
    NCXMOD_MODE_YIN,
    NCXMOD_MODE_FILEYANG,
    NCXMOD_MODE_FILEYIN
} ncxmod_mode_t;


/* Enumeration of the file directory list modes */
typedef enum search_type_t_ {
    SEARCH_TYPE_NONE,
    SEARCH_TYPE_MODULE,
    SEARCH_TYPE_DATA,
    SEARCH_TYPE_SCRIPT
} search_type_t;


/********************************************************************
*                                                                   *
*                       V A R I A B L E S                           *
*                                                                   *
*********************************************************************/
static boolean ncxmod_init_done = FALSE;

static const xmlChar *ncxmod_yuma_home;

static xmlChar *ncxmod_home_cli;

static xmlChar *ncxmod_yuma_home_cli;

static xmlChar *ncxmod_yumadir_path;

static const xmlChar *ncxmod_env_install;

static const xmlChar *ncxmod_home;

static const xmlChar *ncxmod_mod_path = NULL;

static xmlChar *ncxmod_mod_path_cli;

static const xmlChar *ncxmod_alt_path;

static const xmlChar *ncxmod_data_path;

static xmlChar *ncxmod_data_path_cli;

static const xmlChar *ncxmod_run_path;

static xmlChar *ncxmod_run_path_cli;

static boolean ncxmod_subdirs;


/********************************************************************
* FUNCTION is_yang_file
*
* Check the file suffix for .yang
*
* INPUTS:
*    buff == buffer to check
*
* RETURNS:
*   TRUE if .yang extension
*   FALSE otherwise
*********************************************************************/
static boolean
    is_yang_file (const xmlChar *buff)
{
    const xmlChar    *str;
    uint32            count;

    count = xml_strlen(buff);
    if (count < 6) {
        return FALSE;
    }

    if (buff[count-5] != '.') {
        return FALSE;
    }

    str = &buff[count-4];
    return (xml_strcmp(str, YANG_SUFFIX)) ? FALSE : TRUE;

} /* is_yang_file */


/********************************************************************
* FUNCTION is_yin_file
*
* Check the file suffix for .yin
*
* INPUTS:
*    buff == buffer to check
*
* RETURNS:
*   TRUE if .yang extension
*   FALSE otherwise
*********************************************************************/
static boolean
    is_yin_file (const xmlChar *buff)
{
    const xmlChar    *str;
    uint32            count;

    count = xml_strlen(buff);
    if (count < 5) {
        return FALSE;
    }

    if (buff[count-4] != '.') {
        return FALSE;
    }

    str = &buff[count-3];

    return (xml_strcmp(str, YIN_SUFFIX)) ? FALSE : TRUE;

} /* is_yin_file */


/********************************************************************
* FUNCTION prep_dirpath
*
*  Setup the directory path in the buffer
*  Leave it with a trailing path-sep-char so a
*  file name can be added to the buffer
*
* INPUTS:
*    buff == buffer to use for filespec construction
*    bufflen == length of buffer
*    path == first piece of path string (may be NULL)
*    path2 == optional 2nd piece of path string (may be NULL)
*    cnt == address of return count of bytes added to buffer
*
* OUTPUTS:
*    buff filled in with path and path2 if present
*    *cnt == number of bytes added to buff
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    prep_dirpath (xmlChar *buff, 
                  uint32 bufflen,
                  const xmlChar *path, 
                  const xmlChar *path2,
                  uint32 *cnt)
{
    xmlChar *str;
    uint32   pathlen, path2len, pathsep, path2sep, total;
    boolean  needslash;

    *cnt = 0;
    *buff = 0;
    needslash = FALSE;

    if (!path) {
        return NO_ERR;
    }
        
    pathlen = xml_strlen(path);
    if (pathlen) {
        pathsep = (uint32)(path[pathlen-1] == NCXMOD_PSCHAR);
    } else {
        pathsep = 0;
    }

    if (path2) {
        path2len = xml_strlen(path2);
        path2sep = (uint32)(path2len && path2[path2len-1]==NCXMOD_PSCHAR);
    } else {
        path2len = 0;
        path2sep = 0;
    }

    total =  pathlen + path2len;
    if (!pathsep) {
        if (path2 != NULL && *path2 != NCXMOD_PSCHAR) {
            needslash = TRUE;
            total++;
        } else if (path2 == NULL &&
                   !(is_yang_file(path) || is_yin_file(path))) {
            needslash = TRUE;
            total++;
        }
    }

    if (path2 && path2len && !path2sep) {
        total++;
    }

    if (*path == NCXMOD_HMCHAR && path[1] == NCXMOD_PSCHAR) {
        if (!ncxmod_home) {
            return ERR_FIL_BAD_FILENAME;
        } else {
            total += (xml_strlen(ncxmod_home) - 1);
        }
    }

    if (total >= bufflen) {
        log_error("\nncxmod: Path spec too long error. Max: %d Got %u\n",
                  bufflen, total);
        return ERR_BUFF_OVFL;
    }
            
    str = buff;

    if (*path == NCXMOD_HMCHAR && path[1] == NCXMOD_PSCHAR) {
        str += xml_strcpy(str, ncxmod_home);
        str += xml_strcpy(str, &path[1]);
    } else {
        str += xml_strcpy(str, path);
    }

    if (needslash) {
        *str++ = NCXMOD_PSCHAR;
    }

    if (path2 && path2len) {
        str += xml_strcpy(str, path2);
        if (!path2sep) {
            *str++ = NCXMOD_PSCHAR;
        }
    }

    *str = 0;
    *cnt = (uint32)(str-buff);
    return NO_ERR;

}  /* prep_dirpath */


/********************************************************************
* FUNCTION try_module
*
* For YANG and YIN Modules Only!!!
*
* Construct a filespec out of a path name and a module name
* and try to load the filespec as a YANG module
*
* Used in several different modes:
* Buffer mode:
*   input is pre-constructed in 'buff'
* Path Mode
*   input is given in pieces, path and path2 are used
* Current Dir Mode
* Search Mod Path Mode
*   input is given in modname and revision
*
* INPUTS:
*    buff == buffer to use for filespec construction
*    bufflen == length of buffer
*    path == first piece of path string (may be NULL)
*    path2 == optional 2nd piece of path string (may be NULL)
*    modname == module name without file suffix (may be NULL)
*    revision == revision date string (may be NULL)
*    mode == suffix mode
*           == NCXMOD_MODE_YANG if YANG module and suffix is .yang
*           == NCXMOD_MODE_FILEYANG if YANG filespec
*           == NCXMOD_MODE_YIN if YIN module and suffix is .yin
*           == NCXMOD_MODE_FILEYIN if YIN filespec
*    usebuff == use buffer as-is, unless modname is present
*    done == address of return done flag
*    pcb == YANG parser control block (NULL if not used)
*    ptyp == YANG parser source type (YANG_PT_TOP if YANG top module)
*
* OUTPUTS:
*    *done == TRUE if module loaded and status==NO_ERR
*             or status != NO_ERR and fatal error occurred
*             The YANG pcb will also be updated
*
*          == FALSE if file-not-found-error or some non-fatal
*             error so the search should continue if possible
*
*    buff contains the full filespec of the found file
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    try_module (xmlChar *buff, 
                uint32 bufflen,
                const xmlChar *path, 
                const xmlChar *path2,
                const xmlChar *modname,
                const xmlChar *revision,
                ncxmod_mode_t mode,
                boolean usebuff,
                boolean *done,
                yang_pcb_t *pcb,
                yang_parsetype_t ptyp)
{
    xmlChar       *p;
    const xmlChar *suffix;
    uint32         total, modlen, pathlen;
    status_t       res;
    boolean        isyang;
    
    *done = FALSE;
    total = 0;

    switch (mode) {
    case NCXMOD_MODE_YANG:
        suffix = YANG_SUFFIX;
        isyang = TRUE;
        break;
    case NCXMOD_MODE_YIN:
        suffix = YIN_SUFFIX;
        isyang = FALSE;
        break;
    case NCXMOD_MODE_FILEYANG:
        suffix = EMPTY_STRING;
        isyang = TRUE;
        break;
    case NCXMOD_MODE_FILEYIN:
        suffix = EMPTY_STRING;
        isyang = FALSE;
        break;
    default:
        *done = TRUE;
        return SET_ERROR(ERR_INTERNAL_VAL);
    }
    
    if (usebuff) {
        if (modname) {
            /* add module name and maybe revision to end of buffer, 
             * if no overflow 
             */
            pathlen = xml_strlen(buff);
            total =  pathlen + 
                xml_strlen(modname) + 
                ((revision) ? xml_strlen(revision) + 1 : 0) +
                xml_strlen(suffix) + 1;
            if (total >= bufflen) {
                *done = TRUE;
                log_error("\nncxmod: Filename too long error. Max: %d Got %u\n",
                          bufflen,
                          total);
                return ERR_BUFF_OVFL;
            }
            
            p = buff+pathlen;
            p += xml_strcpy(p, modname);
            if (revision) {
                *p++ = YANG_FILE_SEPCHAR;
                p += xml_strcpy(p, revision);
            }
            if (*suffix) {
                *p++ = '.';
                xml_strcpy(p, suffix);
            }
        }
    } else {
        res = prep_dirpath(buff, bufflen, path, path2, &total);
        if (res != NO_ERR) {
            *done = TRUE;
            return res;
        }

        if (modname) {
            modlen = xml_strlen(modname);

            /* construct an complete module filespec 
             * <buff><modname>[@<revision>].<suffix>
             */
            if (*suffix) {
                pathlen = xml_strlen(suffix) + 1;
            } else {
                pathlen = 0;
            }
            if (revision) {
                pathlen += (xml_strlen(revision) + 1);
            }

            if (total + pathlen + modlen >= bufflen) {
                log_info("\nncxmod: Filename too long "
                         "error. Max: %d Got %u",
                         bufflen,
                         total + pathlen + modlen);
                return ERR_BUFF_OVFL;
            }

            /* add module name and suffix */
            p = &buff[total];
            p += xml_strcpy(p, modname);
            if (revision) {
                *p++ = YANG_FILE_SEPCHAR;
                p += xml_strcpy(p, revision);
            }
            if (*suffix) {
                *p++ = '.';
                xml_strcpy(p, suffix);
            }
        }
    }

    /* attempt to load this filespec as a YANG module */
    res = yang_parse_from_filespec(buff, pcb, ptyp, isyang);
    switch (res) {
    case ERR_XML_READER_START_FAILED:
    case ERR_NCX_MISSING_FILE:
        /* not an error, *done == FALSE */
        if (mode==NCXMOD_MODE_YANG || mode==NCXMOD_MODE_YIN) {
            res = NO_ERR;
        }
        break;
    default:
        /* NO_ERR or some other error */
        *done = TRUE;
    }

    return res;

}  /* try_module */


/********************************************************************
* FUNCTION test_file
*
* Construct a filespec out of a path name and a file name
* and try to find the filespec as an data or script file
*
* INPUTS:
*    buff == buffer to use for filespec construction
*    bufflen == length of buffer
*    path == first piece of path string
*    path2 == optional 2nd piece of path string
*    filename == complete filename with or without suffix
* 
* RETURNS:
*    TRUE if file found okay
*    FALSE if file-not-found-error
*********************************************************************/
static boolean
    test_file (xmlChar *buff, 
               uint32 bufflen,
               const xmlChar *path, 
               const xmlChar *path2,
               const xmlChar *filename)
{
    xmlChar    *p;
    uint32      flen, total;
    int         ret;
    status_t    res;
    struct stat statbuf;

    res = prep_dirpath(buff, bufflen, path, path2, &total);
    if (res != NO_ERR) {
        return FALSE;
    }

    flen = xml_strlen(filename);
    if (flen+total >= bufflen) {
        log_error("\nError: Filename too long error. Max: %d Got %u\n",
                  NCXMOD_MAX_FSPEC_LEN, 
                  flen+total);
        return FALSE;
    }

    p = &buff[total];
    p += xml_strcpy(p, filename);

    memset(&statbuf, 0x0, sizeof(statbuf));
    ret = stat((const char *)buff, &statbuf);
    return (ret == 0 && S_ISREG(statbuf.st_mode)) ? TRUE : FALSE;

}  /* test_file */


/********************************************************************
* FUNCTION test_file_make
*
* Construct a filespec out of a path name and a file name
* if the constructed directory is valid
*
* INPUTS:
*    buff == buffer to use for filespec construction
*    bufflen == length of buffer
*    path == first piece of path string
*    path2 == optional 2nd piece of path string
*    filename == complete filename with or without suffix
* 
* RETURNS:
*    status:
*    NO_ERR if directory found OK and buffer constructed OK
*********************************************************************/
static status_t
    test_file_make (xmlChar *buff, 
                    uint32 bufflen,
                    const xmlChar *path, 
                    const xmlChar *path2,
                    const xmlChar *filename)
{
    uint32      flen, total;
    int         ret;
    status_t    res;
    struct stat statbuf;

    res = prep_dirpath(buff, bufflen, path, path2, &total);
    if (res != NO_ERR) {
        return res;
    }

    flen = xml_strlen(filename);
    if (flen+total >= bufflen) {
        log_error("\nError: Filename too long error. Max: %d Got %u\n",
                  NCXMOD_MAX_FSPEC_LEN, 
                  flen+total);
        return ERR_BUFF_OVFL;
    }

    if (path) {
        ret = stat((const char *)buff, &statbuf);

        if (ret == 0 && S_ISDIR(statbuf.st_mode)) {
            xml_strcpy(&buff[total], filename);
            return NO_ERR;
        } else {
            return ERR_NCX_DEF_NOT_FOUND;
        }
    } else {
        xml_strcpy(buff, filename);
        return NO_ERR;
    }

}  /* test_file_make */


/********************************************************************
* FUNCTION test_pathlist
*
* Check the filespec path string for the specified module
* and suffix.  This function does not load any module
* It just finds the specified file.
*
* Subdirs are not checked.  The ENV vars that specify
* directories to search needs to add an entry for each
* dir to search
*
* INPUTS:
*    pathstr == pathstring list to check
*    buff == buffer to use for filespec construction
*    bufflen == length of buffer
*    modname == module name without file suffix
*    modsuffix == file suffix (no dot) [MAY BE NULL]
*
* OUTPUTS:
*    buff contains the complete path to the found file if
*    the return value is TRUE.  Ignore buff contents otherwise
*
* RETURNS:
*    TRUE if file found okay
*    FALSE if file-not-found-error
*********************************************************************/
static boolean
    test_pathlist (const xmlChar *pathlist,
                   xmlChar *buff,
                   uint32 bufflen,
                   const xmlChar *modname,
                   const xmlChar *modsuffix)
{
    const xmlChar *str, *p;
    uint32         len, mlen, slen, dot;
    int            ret;
    struct stat    statbuf;

    mlen = xml_strlen(modname);
    slen = (modsuffix) ? xml_strlen(modsuffix) : 0;
    dot = (uint32)((slen) ? 1 : 0);

    /* go through the path list and check each string */
    str = pathlist;
    while (*str) {
        /* find end of path entry or EOS */
        p = str+1;
        while (*p && *p != ':') {
            p++;
        }
        len = (uint32)(p-str);
        if (len >= bufflen) {
            SET_ERROR(ERR_BUFF_OVFL);
            return FALSE;
        }

        /* copy the next string into buff */
        xml_strncpy(buff, str, len);

        /* make sure string ends with path sep char */
        if (buff[len-1] != NCXMOD_PSCHAR) {
            if (len+1 >= bufflen) {
                SET_ERROR(ERR_BUFF_OVFL);
                return FALSE;
            } else {
                buff[len++] = NCXMOD_PSCHAR;
            }
        }

        /* add the module name and suffix */
        if (len+mlen+dot+slen >= bufflen) {
            SET_ERROR(ERR_BUFF_OVFL);
            return FALSE;
        }

        xml_strcpy(&buff[len], modname);
        if (modsuffix) {
            buff[len+mlen] = '.';
            xml_strcpy(&buff[len+mlen+1], modsuffix);
        }
                
        /* check if the file exists and is readable */
        ret = stat((const char *)buff, &statbuf);
        if (ret == 0) {
            /* match in buff */
            if (S_ISREG(statbuf.st_mode)) {
                return TRUE;
            } else {
                /* should really be an error */
                return FALSE;
            }
        }
    
        /* setup the next path string to try */
        if (*p) {
            str = p+1;   /* one past ':' char */
        } else {
            str = p;        /* already at EOS */
        }
    }

    return FALSE;

}  /* test_pathlist */


/********************************************************************
* FUNCTION test_pathlist_make
*
* Check the filespec path string to find the first
* entry that actually exists.  It does not check
* if the specified user has permission to write
* to this directory.  That must be true or the
* backup will fail
*
* INPUTS:
*    pathstr == pathstring list to check
*    buff == buffer to use for pathspec construction
*    bufflen == length of buffer
*
* OUTPUTS:
*    buff contains the complete path to the found directory if
*    the return value is TRUE.  Ignore buff contents otherwise
*
* RETURNS:
*    TRUE if valid directory found 
*    FALSE otherwise
*********************************************************************/
static boolean
    test_pathlist_make (const xmlChar *pathlist,
                        xmlChar *buff,
                        uint32 bufflen)
{
    const xmlChar *str, *p;
    uint32         len;
    int            ret;
    struct stat    statbuf;

    /* go through the path list and check each string */
    str = pathlist;
    while (*str) {
        /* find end of path entry or EOS */
        p = str+1;
        while (*p && *p != ':') {
            p++;
        }
        len = (uint32)(p-str);
        if (len >= bufflen) {
            SET_ERROR(ERR_BUFF_OVFL);
            return FALSE;
        }

        /* copy the next string into buff */
        xml_strncpy(buff, str, len);

        /* check if this is a directory */
        ret = stat((const char *)buff, &statbuf);
        if (ret == 0) {
            /* match in buff, make sure it is a directory */
            if (S_ISDIR(statbuf.st_mode)) {
                /* make sure string ends with path sep char */
                if (buff[len-1] != NCXMOD_PSCHAR) {
                    if (len+1 >= bufflen) {
                        SET_ERROR(ERR_BUFF_OVFL);
                        return FALSE;
                    } else {
                        buff[len++] = NCXMOD_PSCHAR;
                        buff[len] = 0;
                    }
                }
                return TRUE;
            }
        }
    
        /* setup the next path string to try */
        if (*p) {
            str = p+1;   /* one past ':' char */
        } else {
            str = p;        /* already at EOS */
        }
    }

    return FALSE;

}  /* test_pathlist_make */



/********************************************************************
* FUNCTION check_module_in_dir
*
* Check the directory specified in the buffer
* for any possible for of a YANG/YIN module
*
* INPUTS:
*    buff == buffer to use for filespec construction
*            at the start it contains the path string to use;
*            new directory names will be added to this buffer
*            as the subdirs are searched, until the 'fname' file
*            is found or an error occurs
*    bufflen == size of buff in bytes
*    pathlen == current end of buffer in use marker
*    modname == module name
*    revision == module revision string
*    done == address of return search done flag
*
* OUTPUTS:
*   *done == TRUE if done processing
*            FALSE to keep going
* RETURNS:
*    NO_ERR if file found okay, full filespec in the 'buff' variable
*    OR some error if not found or buffer overflow
*********************************************************************/
static status_t
    check_module_in_dir (xmlChar *buff, 
                         uint32 bufflen,
                         uint32 pathlen,
                         const xmlChar *modname,
                         const xmlChar *revision,
                         boolean *done)
{
    struct stat    statbuf;
    uint32         buffleft;
    int            ret;
    status_t       res;

    /* init locals and return done flag */
    *done = FALSE;
    res = NO_ERR;

    buffleft = bufflen - pathlen;

    /* try YANG first */
    res = yang_copy_filename(modname, revision, &buff[pathlen], buffleft, TRUE);
    if (res != NO_ERR) {
        return res;
    }
    ret = stat((const char *)buff, &statbuf);
    if (ret == 0) {
        *done = TRUE;
        if (S_ISREG(statbuf.st_mode)) {
            return NO_ERR;
        } else {
            return ERR_FIL_BAD_FILENAME;
        }
    }

    /* make sure the path buffer is restored */
    buff[pathlen] = 0;

    /* try YIN next */
    res = yang_copy_filename(modname, revision, &buff[pathlen], buffleft,
                             FALSE);
    if (res != NO_ERR) {
        return res;
    }
    ret = stat((const char *)buff, &statbuf);
    if (ret == 0) {
        *done = TRUE;
        if (S_ISREG(statbuf.st_mode)) {
            return NO_ERR;
        } else {
            return ERR_FIL_BAD_FILENAME;
        }
    }

    /* not done searching yet */
    buff[pathlen] = 0;
    return NO_ERR;

}  /* check_module_in_dir */


/********************************************************************
* FUNCTION search_subdirs
*
* Search any subdirs for the specified module name,
* with optional revision, starting at the specified location,
*
* Modules will be searched as follows:
* if revision == NULL
*    1) modname.yang
*    2) modname.yin
*    3) modname@<??>.yang
*    4) modname@<??>.yin
*
* if revision != NULL
*    1) modname.yang
*    2) modname.yin
*    3) modname@revision.yang
*    4) modname@revision.yin
*
* INPUTS:
*    buff == buffer to use for filespec construction
*            at the start it contains the path string to use;
*            new directory names will be added to this buffer
*            as the subdirs are searched, until the 'fname' file
*            is found or an error occurs
*    bufflen == size of buff in bytes
*    modname == module name
*    revision == module revision string
*    done == address of return search done flag
*
* OUTPUTS:
*   *done == TRUE if done processing
*            FALSE to keep going
* RETURNS:
*    NO_ERR if file found okay, full filespec in the 'buff' variable
*    OR some error if not found or buffer overflow
*********************************************************************/
static status_t
    search_subdirs (xmlChar *buff, 
                    uint32 bufflen,
                    const xmlChar *modname,
                    const xmlChar *revision,
                    boolean *done)
{
    DIR           *dp;
    struct dirent *ep;
    uint32         pathlen, modnamelen, revisionlen, dentlen;
    boolean        dirdone;
    status_t       res;

    /* init locals and return done flag */
    *done = FALSE;
    pathlen = xml_strlen(buff);
    modnamelen = xml_strlen(modname);
    if (revision != NULL) {
        revisionlen = xml_strlen(revision);
    } else {
        revisionlen = 0;
    }
    res = NO_ERR;

    /* check minimal buffer space exists to even start */
    if (pathlen + modnamelen + revisionlen + 2 >= bufflen) {
        *done = TRUE;
        return ERR_BUFF_OVFL;
    } 

    /* make sure path ends with a pathsep char */
    if (buff[pathlen-1] != NCXMOD_PSCHAR) {
        buff[pathlen++] = NCXMOD_PSCHAR;
        buff[pathlen] = 0;
    }

    res = check_module_in_dir(buff, bufflen, pathlen, modname, revision, done);
    if (*done || res != NO_ERR) {
        return res;
    }

    /* try to open the buffer spec as a directory */
    dp = opendir((const char *)buff);
    if (!dp) {
        return NO_ERR;  /* not done yet */
    }

    dirdone = FALSE;
    while (!dirdone) {

        ep = readdir(dp);
        if (!ep) {
            dirdone = TRUE;
            continue;
        }

        /* this field may not be present on all POSIX systems
         * according to the glibc 2.7 documentation!!
         * only using dir file which works on linux. 
         * !!!No support for symbolic links at this time!!!
         *
         * Always skip any directory or file that starts with
         * the dot-char or is named CVS
         */
        dentlen = xml_strlen((const xmlChar *)ep->d_name); 

        /* this dive-first behavior is not really what is desired
         * but do not have a 'stat' function for partial filenames
         * so just going through the directory block in order
         */
        if (ep->d_type == DT_DIR || ep->d_type == DT_UNKNOWN) {
            if (*ep->d_name != '.' && strcmp(ep->d_name, "CVS")) {
                if ((pathlen + dentlen) >= bufflen) {
                    res = ERR_BUFF_OVFL;
                    *done = TRUE;
                    dirdone = TRUE;
                } else {
                    xml_strcpy(&buff[pathlen], 
                               (const xmlChar *)ep->d_name);
                    res = search_subdirs(buff, bufflen, modname, revision, 
                                         done);
                    if (*done) {
                        dirdone = TRUE;
                    } else {
                        /* erase the directory name and keep trying */
                        res = NO_ERR;
                        buff[pathlen] = 0;
                    }
                }
            }
        } 

        if (ep->d_type == DT_REG || ep->d_type == DT_UNKNOWN) {
            if (!xml_strncmp(modname, 
                             (const xmlChar *)ep->d_name,
                             modnamelen)) {
                /* filename is a partial match so check it out
                 * further to see if it is a pattern match;
                 * check if length matches foo@YYYY-MM-DD.yang
                 */
                if ((dentlen == modnamelen + 16) ||
                    (dentlen == modnamelen + 15)) {

                    /* check if the at-sign is 
                     * present in the filespec 
                     */
                    if (ep->d_name[modnamelen] != '@') {
                        continue;
                    }

                    /* check if the revision matches, if specified */
                    if (revision != NULL) {
                        if (xml_strncmp((const xmlChar *)
                                        &ep->d_name[modnamelen+1],
                                        revision, 
                                        revisionlen)) {
                            continue;
                        }
                    }

                    /* check if the file extension is really
                     *.yang or .yin
                     * TBD: search the entire dir for the 
                     * highest valued date string
                     * if the revision == NULL
                     */
                    if (!xml_strcmp((const xmlChar *)
                                    &ep->d_name[modnamelen+12], 
                                    YANG_SUFFIX) ||
                        !xml_strcmp((const xmlChar *)
                                    &ep->d_name[modnamelen+12], 
                                    YIN_SUFFIX)) {
                        dirdone = TRUE;
                        *done = TRUE;
                        if ((pathlen + dentlen) >= bufflen) {
                            res = ERR_BUFF_OVFL;
                        } else {
                            res = NO_ERR;
                            xml_strcpy(&buff[pathlen], 
                                       (const xmlChar *)ep->d_name);
                        }
                    }
                }
            }
        }
    }

    (void)closedir(dp);

    return res;

}  /* search_subdirs */


/********************************************************************
* FUNCTION list_subdirs
*
* List the relevant files in the subdir
*
* INPUTS:
*    buff == buffer to use for filespec construction
*            at the start it contains the path string to use;
*            new directory names will be added to this buffer
*            as the subdirs are traversed
*    bufflen == size of buff in bytes
*    searchtype == enum for the search type to use
*    helpmode == help mode to use (BRIEF, NORMAL, FULL)
*    logstdout == TRUE for log_stdout, FALSE for log_write
*    dive == TRUE to list subdirs; FALSE to list just the
*            specified directory
*
* RETURNS:
*    NO_ERR if file found okay, full filespec in the 'buff' variable
*    OR some error if not found or buffer overflow
*********************************************************************/
static status_t
    list_subdirs (xmlChar *buff,
                  uint32 bufflen,
                  search_type_t searchtype,
                  help_mode_t helpmode,
                  boolean logstdout,
                  boolean dive)
{
    DIR           *dp;
    struct dirent *ep;
    xmlChar       *str;
    uint32         pathlen, dentlen;
    boolean        dirdone, isyang, isyin;
    status_t       res;

    res = NO_ERR;
    dentlen = 0;
    pathlen = xml_strlen(buff);

    /* try to open the buffer spec as a directory */
    dp = opendir((const char *)buff);
    if (!dp) {
        return NO_ERR;  /* not done yet */
    }

    /* list top-down:
     * first all the files first from this directory
     * then files from any subdirs
     */
    dirdone = FALSE;
    while (!dirdone) {

        ep = readdir(dp);
        if (!ep) {
            dirdone = TRUE;
            continue;
        }

        if (ep->d_type == DT_REG || ep->d_type == DT_UNKNOWN) {

            /* skip files that start with a dot char */
            if (*ep->d_name == '.') {
                continue;
            }

            isyang = is_yang_file((const xmlChar *)ep->d_name);
            isyin = is_yin_file((const xmlChar *)ep->d_name);

            /* check if this file needs to be listed */
            switch (searchtype) {
            case SEARCH_TYPE_MODULE:
                if (isyang || isyin) {
                    break;
                }
                continue;
            case SEARCH_TYPE_DATA:
            case SEARCH_TYPE_SCRIPT:
                if (isyang || isyin) {
                    continue;
                }
                break;
            case SEARCH_TYPE_NONE:
            default:
                dirdone = TRUE;
                SET_ERROR(ERR_INTERNAL_VAL);
                continue;

            }

            /* list the file */
            if (logstdout) {
                log_stdout_indent(NCX_DEF_INDENT);
                if (helpmode == HELP_MODE_FULL) {
                    log_stdout((const char *)buff);
                }
                log_stdout(ep->d_name);
                if (helpmode != HELP_MODE_BRIEF) {
                    ;
                }
            } else {
                log_indent(NCX_DEF_INDENT);
                if (helpmode == HELP_MODE_FULL) {
                    log_write((const char *)buff);
                }
                log_write(ep->d_name);
                if (helpmode != HELP_MODE_BRIEF) {
                    ;
                }
            }
        }
    }

    (void)closedir(dp);

    if (!dive) {
        return NO_ERR;
    }

    /* try to open the buffer spec as a directory */
    dp = opendir((const char *)buff);
    if (!dp) {
        return NO_ERR;
    }

    /* go through again but this time just dive into the subdirs */
    dirdone = FALSE;
    while (!dirdone) {

        ep = readdir(dp);
        if (!ep) {
            dirdone = TRUE;
            continue;
        }

        /* this field may not be present on all POSIX systems
         * according to the glibc 2.7 documentation!!
         * only using dir file which works on linux. 
         * !!!No support for symbolic links at this time!!!
         * If d_type == DT_UNKNOWN then guessing this is a file
         *
         * Always skip any directory or file that starts with
         * the dot-char or is named CVS
         */
        dentlen = xml_strlen((const xmlChar *)ep->d_name); 

        /* this dive-first behavior is not really what is desired
         * but do not have a 'stat' function for partial filenames
         * so just going through the directory block in order
         */
        if (ep->d_type == DT_DIR || ep->d_type == DT_UNKNOWN) {
            if (*ep->d_name != '.' && strcmp(ep->d_name, "CVS")) {
                if ((pathlen + dentlen + 2) >= bufflen) {
                    res = ERR_BUFF_OVFL;
                    dirdone = TRUE;
                } else {
                    /* make sure last dirspec ends witha path-sep-char */
                    if (pathlen > 0 && buff[pathlen-1] != NCXMOD_PSCHAR) {
                        buff[pathlen] = NCXMOD_PSCHAR;
                        str = &buff[pathlen+1];
                        str += xml_strcpy(str, (const xmlChar *)ep->d_name);
                    } else {
                        str = &buff[pathlen];
                        str += xml_strcpy(str, (const xmlChar *)ep->d_name);
                    }
                    *str++ = NCXMOD_PSCHAR;
                    *str = '\0';

                    res = list_subdirs(buff, bufflen, searchtype, helpmode,
                                       logstdout, TRUE);
                    /* erase the filename and keep trying */
                    buff[pathlen] = 0;
                    if (res != NO_ERR) {
                        dirdone = TRUE;
                        continue;
                    }
                }
            }
        }
    }

    (void)closedir(dp);
    return res;

}  /* list_subdirs */


/********************************************************************
* FUNCTION list_pathlist
*
* Check the filespec path string and list files in each dir
*
* INPUTS:
*    pathstr == pathstring list to check
*    buff == scratch buffer to use
*    bufflen == length of buff
*    searchtype == search type to use
*    helpmode == verbosity mode (BRIEF, NORMAL, FULL)
*    logstdout == TRUE for log_stdout, FALSE for log_write
*
* RETURNS:
*    status
*********************************************************************/
static status_t
    list_pathlist (const xmlChar *pathlist,
                   xmlChar *buff,
                   uint32 bufflen,
                   search_type_t  searchtype,
                   help_mode_t helpmode,
                   boolean logstdout)
{
    const xmlChar *str, *p;
    uint32         len;
    status_t       res;

    res = NO_ERR;
    str = pathlist;

    /* go through the path list and check each string */
    while (*str) {
        /* find end of path entry or EOS */
        p = str+1;
        while (*p && *p != ':') {
            p++;
        }
        len = (uint32)(p-str);
        if (len >= bufflen) {
            return ERR_BUFF_OVFL;
        }

        /* copy the next string into buff */
        xml_strncpy(buff, str, len);

        /* make sure string ends with path sep char */
        if (buff[len-1] != NCXMOD_PSCHAR) {
            if (len+1 >= bufflen) {
                return ERR_BUFF_OVFL;
            } else {
                buff[len++] = NCXMOD_PSCHAR;
            }
        }

        /* list all the requested files in this path */
        res = list_subdirs(buff, bufflen, searchtype, helpmode, logstdout,
                           FALSE);

        if (res != NO_ERR) {
            return res;
        }
    
        /* setup the next path string to try */
        if (*p) {
            str = p+1;   /* one past ':' char */
        } else {
            str = p;        /* already at EOS */
        }
    }

    return res;

}  /* list_pathlist */


/********************************************************************
* FUNCTION add_failed
*
* Add a yang_node_t entry to the pcb->failedQ
*
* INPUTS:
*   modname == failed module name
*   revision == failed revision date (may be NULL)
*   pcb == parser control block
*   res == final result status for the failed module
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    add_failed (const xmlChar *modname,
                const xmlChar *revision,
                yang_pcb_t *pcb,
                status_t res)
{
    yang_node_t *node;

    /* do not save failed modules based on a failed search
     * or if the module is not really being added to the registry
     */
    if (pcb->searchmode || pcb->parsemode) {
        return NO_ERR;
    }

    node = yang_new_node();
    if (!node) {
        return ERR_INTERNAL_MEM;
    }

    node->failed = xml_strdup(modname);
    if (!node->failed) {
        yang_free_node(node);
        return ERR_INTERNAL_MEM;
    }

    if (revision) {
        node->failedrev = xml_strdup(revision);
        if (!node->failed) {
            yang_free_node(node);
            return ERR_INTERNAL_MEM;
        }
    }

    node->name = node->failed;
    node->revision = node->failedrev;
    node->res = res;
    dlq_enque(node, &pcb->failedQ);
    return NO_ERR;

} /* add_failed */


/********************************************************************
* FUNCTION check_module_path
*
*  Check the specified path for a YANG module file
*
* INPUTS:
*   path == starting path to check
*   buff == buffer to use for the filespec
*   bufflen == size of 'buff' in bytes
*   modname == module name to find (no file suffix)
*   revision == module revision date (may be NULL)
*   pcb == parser control block in progress
*   ptyp == parser source type
*   usepath == TRUE if path should be used directly
*              FALSE if the path should be appended with the 'modules' dir
*   done == address of return done flag
*
* OUTPUTS:
*   *done == TRUE if file found or fatal error
*    file completely processed in try_module if file found
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    check_module_path (const xmlChar *path,
                       xmlChar *buff,
                       uint32 bufflen,
                       const xmlChar *modname,
                       const xmlChar *revision,
                       yang_pcb_t *pcb,
                       yang_parsetype_t ptyp,
                       boolean usepath,
                       boolean *done)
{
    const xmlChar  *path2;
    uint32          total;
    status_t        res, res2;

    *done = FALSE;
    res = NO_ERR;
    res2 = NO_ERR;
    /* T == use path directly, leave path2 NULL
     * F == add 'modules' to end of path by setting path2
     */
    path2 = (usepath) ? NULL : NCXMOD_DIR;

    total = xml_strlen(path);
    if (total >= bufflen) {
        *done = TRUE;
        return ERR_BUFF_OVFL;
    }

    if (ncxmod_subdirs) {
        res = prep_dirpath(buff, bufflen, path, path2, &total);
        if (res != NO_ERR) {
            *done = TRUE;
            return res;
        }

        /* try YANG or YIN file */
        res = search_subdirs(buff, bufflen, modname, revision, done);
        if (*done && res == NO_ERR) {

            res = try_module(buff, 
                             bufflen, 
                             NULL, 
                             NULL,
                             NULL, 
                             NULL,
                             yang_fileext_is_yang(buff) ? 
                             NCXMOD_MODE_FILEYANG : 
                             NCXMOD_MODE_FILEYIN,
                             TRUE, 
                             done, 
                             pcb, 
                             ptyp);
            if (res != NO_ERR) {
                res2 = add_failed(modname, 
                                  revision,
                                  pcb, 
                                  res);
            }
        }
        return (res != NO_ERR) ? res : res2;
    }

    /* else subdir searches not allowed
     * check for YANG file in the current path
     */
    res = try_module(buff,
                     bufflen, 
                     path,
                     path2,
                     modname,
                     revision,
                     NCXMOD_MODE_YANG,
                     FALSE, 
                     done,
                     pcb,
                     ptyp);

    if (!*done && res == NO_ERR) {
        /* check for YIN file in the current path */
        res = try_module(buff,
                         bufflen, 
                         path,
                         path2,
                         modname,
                         revision,
                         NCXMOD_MODE_YIN,
                         FALSE, 
                         done,
                         pcb,
                         ptyp);
    }

    if (*done && res != NO_ERR) {
        res2 = add_failed(modname,
                          revision,
                          pcb,
                          res);
    }
    return (res != NO_ERR) ? res : res2;

}  /* check_module_path */


/********************************************************************
* FUNCTION check_module_pathlist
*
*  Check a list of pathnames for the specified path of 
*  a YANG module file
*
*  Example:   path1:path2:path3
*
* INPUTS:
*   pathlist == formatted string containing list of path strings
*   buff == buffer to use for the filespec
*   bufflen == size of 'buff' in bytes
*   modname == module name to find (no file suffix)
*   revision == module revision date (may be NULL)
*   pcb == parser control block in progress
*   ptyp == parser source type
*   done == address of return done flag
*
* OUTPUTS:
*   *done == TRUE if file found or fatal error
*    file completely processed in try_module if file found
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    check_module_pathlist (const xmlChar *pathlist,
                           xmlChar *buff,
                           uint32 bufflen,
                           const xmlChar *modname,
                           const xmlChar *revision,
                           yang_pcb_t *pcb,
                           yang_parsetype_t ptyp,
                           boolean *done)
{
    const xmlChar  *str, *p;
    xmlChar        *pathbuff;
    uint32          pathbufflen, pathlen;
    status_t        res;

    pathbufflen = NCXMOD_MAX_FSPEC_LEN+1;
    pathbuff = m__getMem(pathbufflen);
    if (!pathbuff) {
        *done = TRUE;
        return ERR_INTERNAL_MEM;
    }

    /* go through the path list and check each string */
    str = pathlist;
    while (*str) {
        /* find end of path entry or EOS */
        p = str+1;
        while (*p && *p != ':') {
            p++;
        }

        pathlen = (uint32)(p-str);
        if (pathlen >= pathbufflen) {
            *done = TRUE;
            m__free(pathbuff);
            return ERR_BUFF_OVFL;
        }

        /* copy the next string into the path buffer */
        xml_strncpy(pathbuff, str, pathlen);
        res = check_module_path(pathbuff, 
                                buff,
                                bufflen,
                                modname, 
                                revision,
                                pcb,
                                ptyp,
                                TRUE,
                                done);
        if (*done) {
            m__free(pathbuff);
            return res;
        }
    
        /* setup the next path string to try */
        if (*p) {
            str = p+1;   /* one past ':' char */
        } else {
            str = p;        /* already at EOS */
        }
    }

    m__free(pathbuff);
    *done = FALSE;
    return NO_ERR;

}  /* check_module_pathlist */


/********************************************************************
* FUNCTION search_module_path
*
*  Check the specified path for all YANG files
*
* INPUTS:
*   path == starting path to check
*   buff == buffer to use for the filespec
*   bufflen == size of 'buff' in bytes
*   callback == callback function to use for process_subdir
*   cookie == parameter for the callback
*
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    search_module_path (const xmlChar *path,
                        xmlChar *buff,
                        uint32 bufflen,
                        ncxmod_callback_fn_t callback,
                        void *cookie)



{
    status_t        res;
    uint32          total;

    total = 0;
    res = prep_dirpath(buff, bufflen, path, NCXMOD_DIR, &total);
    if (res == NO_ERR) {
        res = ncxmod_process_subtree((const char *)buff, 
                                     callback, 
                                     cookie);
    }
    return res;

}  /* search_module_path */


/********************************************************************
* FUNCTION search_module_pathlist
*
*  Check a list of pathnames for all the YANG files in the path
*
*  Example:   path1:path2/foo/bar:path3
*
* INPUTS:
*   pathlist == formatted string containing list of path strings
*   callback == callback function to use for process_subdir
*   cookie == parameter for the callback
*
* RETURNS:
*   status
*********************************************************************/
static status_t
    search_module_pathlist (const xmlChar *pathlist,
                            ncxmod_callback_fn_t callback,
                            void *cookie)

{
    const xmlChar  *str, *p;
    xmlChar        *pathbuff;
    uint32          pathbufflen, pathlen;
    status_t        res;

    pathbufflen = NCXMOD_MAX_FSPEC_LEN+1;
    pathbuff = m__getMem(pathbufflen);
    if (!pathbuff) {
        return ERR_INTERNAL_MEM;
    }

    /* go through the path list and check each string */
    str = pathlist;
    res = NO_ERR;

    while (*str) {
        /* find end of path entry or EOS */
        p = str+1;
        while (*p && *p != ':') {
            p++;
        }

        pathlen = (uint32)(p-str);
        if (pathlen >= pathbufflen) {
            m__free(pathbuff);
            return ERR_BUFF_OVFL;
        }

        /* copy the next string into the path buffer */
        xml_strncpy(pathbuff, str, pathlen);

        res = ncxmod_process_subtree((const char *)pathbuff, 
                                     callback, 
                                     cookie);
        if (res != NO_ERR) {
            m__free(pathbuff);
            return res;
        }
    
        /* setup the next path string to try */
        if (*p) {
            str = p+1;   /* one past ':' char */
        } else {
            str = p;        /* already at EOS */
        }
    }

    m__free(pathbuff);
    return NO_ERR;

}  /* search_module_pathlist */



/********************************************************************
* FUNCTION is_module_file
*
* Determine if the module is a file
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*
* OUTPUTS:

* RETURNS:
*   True if the module is a file
*
*********************************************************************/
static boolean
    is_module_file( const xmlChar *modname )
{
    boolean isfile = FALSE;

    /* check which form of input is present, module name or filespec */
    if ((*modname == '.') || (*modname == NCXMOD_PSCHAR)) {
        return  TRUE;
    } else {
        /* if a dir sep char is in the string it is automatically treated as an 
         * absolute filespec, even if it doesn't have a file suffix */
        const xmlChar  *str = modname;
        while (*str && *str != NCXMOD_PSCHAR) {
            ++str;
        }

        if (*str) {
            isfile = TRUE;
        }
    }

    return isfile;
}

/********************************************************************
* FUNCTION determine_mode
*
* Determine if the module is a YANG or YIN file extension
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   modlen == length of modname string
*
* OUTPUTS:
*
* RETURNS:
*   True if teh module is a file
*
*********************************************************************/
static ncxmod_mode_t
    determine_mode( const xmlChar* modname, 
                    const uint32 modlen )
{
    ncxmod_mode_t   mode = NCXMOD_MODE_NONE;
    const xmlChar  *str = modname;

    // find the start of the file suffix, if any
    str = &modname[modlen];
    while (str > modname && *str != '.') {
        --str;
    }

    /* try to find a .yang file suffix */
    if (*str == '.') {
        /* since the dot-char is allowed in YANG identifier names
         * only treat this string with a dot in it as a file if
         * it has a YANG file extension
         */
        if (!xml_strcmp(str+1, YANG_SUFFIX)) {
            mode = NCXMOD_MODE_FILEYANG;
        } else if (!xml_strcmp(str+1, YIN_SUFFIX)) {
            mode = NCXMOD_MODE_FILEYIN;
        }
    }

    if ( NCXMOD_MODE_NONE == mode ) {
        if ( is_module_file ( modname ) ) {
            mode = NCXMOD_MODE_FILEYIN;
        }
    }

    return mode;
}

/********************************************************************
* FUNCTION try_module_filespec
*
* try to load a module from a file
*
* INPUTS:
*   mode the file mode (YANG or YIN)
*
* OUTPUTS:

* RETURNS:
*   True if teh module is a file
*
*********************************************************************/
static status_t
    try_module_filespec( const ncxmod_mode_t  mode,
                         const xmlChar       *modname,
                         const uint32         modlen,
                         yang_pcb_t          *pcb,
                         yang_parsetype_t     ptyp,
                         ncx_module_t       **retmod)
{
    status_t res = ERR_NCX_MISSING_FILE;
    xmlChar *buff;
    boolean         done = FALSE;

    /* the try_module function expects the first parm to be a writable 
     * buffer, even though in this case there will be nothing altered in the
     * module name string.  Need to copy it instead of pass it directly :-( */
    buff = xml_strdup(modname);
    if ( !buff ) {
        return ERR_INTERNAL_MEM;
    }

    res = try_module( buff, modlen, NULL, NULL, NULL, NULL, 
                      mode, TRUE, &done, pcb, ptyp);

    if ( ERR_NCX_MISSING_FILE == res ) {
        log_error("\nError: file not found (%s)\n", modname);
    } else if ( res == NO_ERR && retmod ) {
        *retmod = pcb->top;
    }

    m__free(buff);
    return res;
}
 
/********************************************************************
* FUNCTION load_module
*
* Determine the location of the specified module
* and then load it into the system, if not already loaded
*
* Module Search order:
*   1) filespec == try that only and exit
*   2) current directory
*   3) YUMA_MODPATH environment var (or set by modpath CLI var)
*   4) HOME/modules directory
*   5) YUMA_HOME/modules directory
*   6) YUMA_INSTALL/modules directory OR
*   7) default install module location, which is '/usr/share/yuma/modules'
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*   pcb == parser control block
*   ptyp == current parser source type
*   retmod == address of return module (may be NULL)
*
* OUTPUTS:
*   if non-NULL:
*    *retmod == pointer to requested module version
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    load_module (const xmlChar *modname,
                 const xmlChar *revision,
                 yang_pcb_t *pcb,
                 yang_parsetype_t ptyp,
                 ncx_module_t **retmod)
{
    ncxmod_mode_t   mode = NCXMOD_MODE_NONE;
    xmlChar        *buff;
    uint32          modlen;
    uint32          bufflen = 0;
    status_t        res = NO_ERR;
    boolean         done = FALSE;

    if ( !modname || !pcb )
    {
        return ERR_INTERNAL_PTR;
    }

    if (LOGDEBUG2) {
        log_debug2("\nAttempting to load module '%s'", modname);
        if (revision) {
            log_debug2(" r:%s", revision);
        }
    } else if (LOGDEBUG) {
        log_debug("\nload_module called for '%s'", modname);
    }

    modlen = xml_strlen(modname);

    if (retmod) {
        *retmod = NULL;
    }

    /* find the start of the file name extension, expecting .yang or .yin */
    mode = determine_mode( modname, modlen );
    if ( NCXMOD_MODE_FILEYANG == mode || NCXMOD_MODE_FILEYIN == mode ) {
        /* 1) if parameter is a filespec, then try it and exit if it does not 
         * work, instead of trying other directories */
        return try_module_filespec( mode, modname, modlen, pcb, ptyp, retmod );
    }

    /* the module name is not a file; the revision may be relevant now;
     * make sure the module name is valid */
    if ( !ncx_valid_name( modname, modlen ) ) {
        log_error("\nError: Invalid module name (%s)\n", modname);
        res = add_failed(modname, revision, pcb, res);
        if ( NO_ERR != res ) {
            return res;
        } else {
            return ERR_NCX_INVALID_NAME;
        }
    }

    /* check if the module is already loaded (in the current ncx_modQ) skip if 
     * this is yangdump converting a YANG file to a YIN file or 
     * yangcli_autoload parsing a server module */
    if ((ptyp != YANG_PT_INCLUDE) &&
        !(pcb->parsemode && ptyp == YANG_PT_TOP) &&
        !(pcb->savetkc && pcb->tkc == NULL)) {
        ncx_module_t   *testmod;

        testmod = ncx_find_module(modname, revision);
        if (testmod) {
            log_debug2( "\nncxmod: Using module '%s' already loaded", modname );
            if (!pcb->top) {
                pcb->top = testmod;
                pcb->topfound = TRUE;
            }
            if (retmod) {
                *retmod = testmod;
            }
            return testmod->status;
        } else {
            /*** hack for now, check if this is ietf-netconf ***/
            if (!xml_strcmp(modname, NCXMOD_IETF_NETCONF)) {
                /* check if yuma-netconf already loaded */
                testmod = ncx_find_module(NCXMOD_YUMA_NETCONF, NULL);
                if (testmod) {
                    /* use yuma-netconf instead of ietf-netconf */
                    log_debug( "\nncxmod: cannot load 'ietf-netconf'; "
                               "'yuma-netconf' already loaded" );
                    if (!pcb->top) {
                        pcb->top = testmod;
                        pcb->topfound = TRUE;
                    }
                    if (retmod) {
                        *retmod = testmod;
                    }
                    return testmod->status;
                }
            }
        }
    }

    /* get a temp buffer to construct filespecs */
    bufflen = NCXMOD_MAX_FSPEC_LEN+1;
    buff = m__getMem(bufflen);
    if (!buff) {
        return ERR_INTERNAL_MEM;
    } else {
        *buff = 0;
    }

    /* 2) try alt_path variable if set; used by yangdiff */
    if ( ncxmod_alt_path) {
        res = check_module_path( ncxmod_alt_path, buff, bufflen, modname, 
                                 revision, pcb, ptyp, TRUE, &done );
    }

    if (ncx_get_cwd_subdirs()) {
        /* CHECK THE CURRENT DIR AND ANY SUBDIRS
         * 3) try cur working directory and subdirs if the subdirs parameter
         *    is true
         * check before the modpath, which can cause the wrong version to be 
         * picked, depending * on the CWD used by the application.  */
        if (!done) {
            res = check_module_pathlist( (const xmlChar *)".", buff, bufflen,
                                         modname, revision, pcb, ptyp, &done );
        }
    } else {
        /* CHECK THE CURRENT DIR BUT NOT ANY SUBDIRS
         * 3a) try as module in current dir, YANG format */
        if (!done) {
            res = try_module( buff, bufflen, NULL, NULL, modname, revision, 
                              NCXMOD_MODE_YANG, FALSE, &done, pcb, ptyp );
        }

        /* 3b) try as module in current dir, YIN format  */
        if (!done) {
            res = try_module( buff, bufflen, NULL, NULL, modname, revision,
                              NCXMOD_MODE_YIN, FALSE, &done, pcb, ptyp);
        }
    }

    /* 4) try YUMA_MODPATH environment variable if set */
    if (!done && ncxmod_mod_path) {
        res = check_module_pathlist( ncxmod_mod_path, buff, bufflen, modname, 
                                     revision, pcb, ptyp, &done );
    }

    /* 5) HOME/modules directory */
    if (!done && ncxmod_home) {
        res = check_module_path( ncxmod_home, buff, bufflen, modname,
                                 revision, pcb, ptyp, FALSE, &done );
    }

    /* 6) YUMA_HOME/modules directory */
    if (!done && ncxmod_yuma_home) {
        res = check_module_path( ncxmod_yuma_home, buff, bufflen, modname, 
                                 revision, pcb, ptyp, FALSE, &done );
    }

    /* 7) YUMA_INSTALL/modules directory or default install path
     *    If this envvar is set then the default install path will not
     *    be tried
     */
    if (!done) {
        if (ncxmod_env_install) {
            res = check_module_path( ncxmod_env_install, buff, bufflen, modname,
                                     revision, pcb, ptyp, FALSE, &done );
        } else {
            res = check_module_path( NCXMOD_DEFAULT_INSTALL, buff, bufflen, 
                                     modname, revision, pcb, ptyp, FALSE, 
                                     &done );
        }
    }

    if (res != NO_ERR || !done) {
        if (!done) {
            res = ERR_NCX_MOD_NOT_FOUND;
        }

        ( void ) add_failed(modname, revision, pcb, res);
    }

    m__free(buff);

    if (done) {
        if ( ( res == NO_ERR || ptyp == YANG_PT_INCLUDE ) && retmod ) {
            if (pcb->retmod != NULL) {
                *retmod = pcb->retmod;
            } else if (ptyp == YANG_PT_TOP) {
                *retmod = pcb->top;
            } else {
                SET_ERROR(ERR_INTERNAL_VAL);
            }
        } else if (res != NO_ERR && pcb->retmod) {
            log_debug( "\nFree retmod import %p (%s)", 
                       pcb->retmod, pcb->retmod->name);
            ncx_free_module(pcb->retmod);
            pcb->retmod = NULL;
        } /* else pcb->top will get deleted in yang_free_pcb */
    }

    return res;
}  /* load_module */


/********************************************************************
* FUNCTION has_mod_ext
*
* Check if the filespec ends in '.yang' or '.yin'
*
* INPUTS:
*    filespec == file spec string to check
*
* RETURNS:
*    TRUE if YANG file extension found
*       and non-zero filename\
*    FALSE otherwise
*********************************************************************/
static boolean
    has_mod_ext (const char *filespec)
{
    const char *p;

    p = filespec;

    while (*p) {
        p++;
    }

    while (p>filespec && (*p != '.')) {
        p--;
    }

    if (p==filespec) {
        return FALSE;
    }

    if (!strcmp(p+1, (const char *)YANG_SUFFIX)) {
        return TRUE;
    }

    if (!strcmp(p+1, (const char *)YIN_SUFFIX)) {
        return TRUE;
    }

    return FALSE;
    
}  /* has_mod_ext */


/********************************************************************
* FUNCTION process_subtree
*
* Search the entire specified subtree, looking for YANG
* modules.  Invoke the callback function for each module
* file found
*
* INPUTS:
*    buff == working filespec buffer containing the dir spec 
*            to start with
*    bufflen == maxsize of buff, in bytes
*    callback == address of the ncxmod_callback_fn_t function
*         to use for this traveral
*    cookie == cookie to pass to each invocation of the callback
*
* RETURNS:
*    NO_ERR if file found okay, full filespec in the 'buff' variable
*    OR some error if not found or buffer overflow
*********************************************************************/
static status_t
    process_subtree (char *buff, 
                     uint32 bufflen,
                     ncxmod_callback_fn_t callback,
                     void *cookie)
{
    DIR           *dp;
    struct dirent *ep;
    uint32         pathlen;
    boolean        dirdone;
    status_t       res;


    res = NO_ERR;

    pathlen = xml_strlen((const xmlChar *)buff);
    if (!pathlen) {
        return NO_ERR;
    }

    /* make sure a min-length YANG file can be added (x.yang) */
    if ((pathlen + 8) >= bufflen) {
        log_error("\nError: pathspec too long '%s'\n", buff);
        return ERR_BUFF_OVFL;
    } 

    /* make sure dir-sep char is in place */
    if (buff[pathlen-1] != NCXMOD_PSCHAR) {
        buff[pathlen++] = NCXMOD_PSCHAR;
        buff[pathlen] = 0;
    }

    /* try to open the buffer spec as a directory */
    dp = opendir(buff);
    if (!dp) {
#if 0
        log_error("\nError: open directory '%s' failed\n", buff);
        return ERR_OPEN_DIR_FAILED;
#else
        return NO_ERR;
#endif
    }

    dirdone = FALSE;
    while (!dirdone && res==NO_ERR) {

        ep = readdir(dp);
        if (!ep) {
            dirdone = TRUE;
            continue;
        }

        /* this field may not be present on all POSIX systems
         * according to the glibc 2.7 documentation!!
         * only using dir file which works on linux. 
         * !!!No support for symbolic links at this time!!!
         */
        if (ep->d_type == DT_DIR || ep->d_type == DT_UNKNOWN) {
            if ((*ep->d_name != '.') && strcmp(ep->d_name, "CVS")) {
                if ((pathlen + 
                     xml_strlen((const xmlChar *)ep->d_name)) >=  bufflen) {
                    res = ERR_BUFF_OVFL;
                } else {
                    strncpy(&buff[pathlen], ep->d_name, bufflen-pathlen);
                    res = process_subtree(buff, bufflen, callback, cookie);
                    buff[pathlen] = 0;
                }
            }
        }

        if (ep->d_type == DT_REG || ep->d_type == DT_UNKNOWN) {
            if ((*ep->d_name != '.') && has_mod_ext(ep->d_name)) {
                if ((pathlen + 
                     xml_strlen((const xmlChar *)ep->d_name)) >=  bufflen) {
                    res = ERR_BUFF_OVFL;
                } else {
                    strncpy(&buff[pathlen], ep->d_name, bufflen-pathlen);
                    res = (*callback)(buff, cookie);
                }
            }
        }
    }

    (void)closedir(dp);

    return res;

}  /* process_subtree */


/********************************************************************
* FUNCTION try_load_module
*
* Try 1 to 3 forms of the YANG filespec to
* locate the specified module
*
* INPUTS:
*   pcb == parser control block to use
*   ptyp == parser mode type to use
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*   retmod == address of return module (may be NULL)
*
* OUTPUTS:
*   if non-NULL:
*    *retmod == pointer to requested module version
*
* RETURNS:
*   status
*********************************************************************/
static status_t 
    try_load_module (yang_pcb_t *pcb,
                     yang_parsetype_t ptyp,
                     const xmlChar *modname,
                     const xmlChar *revision,
                     ncx_module_t **retmod)
{
    ncx_module_t    *testmod;
    status_t         res;

#ifdef DEBUG
    if (!modname) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    testmod = NULL;

    /* need to choose the correct match mode from the start
     * so that foo@rev-date.yang early in the search path
     * does not get passed over for a generic foo.yang
     * later on in the search path
     */
    res = load_module(modname, revision, pcb, ptyp, &testmod);

    if (res == ERR_NCX_MOD_NOT_FOUND) {
        if (revision && *revision) {
            /* try filenames without revision dates in them */
            res = load_module(modname, NULL, pcb, ptyp, &testmod);
            if (res == NO_ERR && testmod) {
                if (testmod->version == NULL) {
                    /* asked for a specific revision; 
                     * got a generic revision instead
                     * rejected!; return error
                     */
                    res = ERR_NCX_WRONG_VERSION;
                } else if (yang_compare_revision_dates(revision,
                                                       testmod->version)) {
                    /* error should already be reported */
                    res = ERR_NCX_WRONG_VERSION;
                } /* else got correct revision */
            }
        }
    }

    if (res == NO_ERR && testmod && testmod->errors) {
        log_debug("\nParser returned OK but module '%s' has errors",
                  testmod->name);
        res = ERR_NCX_OPERATION_FAILED;
        if (!testmod->ismod && (pcb->top != testmod)) {
            ncx_free_module(testmod);
        }
    } else if (retmod) {
        *retmod = testmod;
    }

    return res;

}  /* try_load_module */


/********************************************************************
* FUNCTION new_temp_filcb
* 
* Malloc a new temporary file control block
*
* RETURNS:
*  malloced and initialized struct
*********************************************************************/
static ncxmod_temp_filcb_t *
    new_temp_filcb (void)

{
    ncxmod_temp_filcb_t *newcb;

    newcb = m__getObj(ncxmod_temp_filcb_t);
    if (newcb == NULL) {
        return NULL;
    }

    memset(newcb, 0x0, sizeof(ncxmod_temp_filcb_t));
    return newcb;

}  /* new_temp_filcb */


/********************************************************************
* FUNCTION free_temp_filcb
* 
* Clean and free a temporary file control block
*
* INPUTS:
*    filcb == temporary file control block to free
*
*********************************************************************/
static void
    free_temp_filcb (ncxmod_temp_filcb_t *filcb)
{
    int   retval;

    if (filcb->source) {
        retval = remove((const char *)filcb->source);
        if (retval < 0) {
            log_error("\nError: could not delete temp file '%s' (%s)\n",
                      filcb->source,
                      get_error_string(errno_to_status()));
        }
        m__free(filcb->source);
    }
    m__free(filcb);

}  /* free_temp_filcb */


/********************************************************************
* FUNCTION new_temp_sescb
* 
* Malloc a new temporary session files control block
*
* RETURNS:
*  malloced and initialized struct
*********************************************************************/
static ncxmod_temp_sescb_t *
    new_temp_sescb (void)

{
    ncxmod_temp_sescb_t *newcb;

    newcb = m__getObj(ncxmod_temp_sescb_t);
    if (newcb == NULL) {
        return NULL;
    }

    memset(newcb, 0x0, sizeof(ncxmod_temp_sescb_t));
    dlq_createSQue(&newcb->temp_filcbQ);
    return newcb;

}  /* new_temp_sescb */


/********************************************************************
* FUNCTION free_temp_sescb
* 
* Clean and free a new temporary session files control block
*
* INPUTS:
*    sescb == temp files session control block to free
*
*********************************************************************/
static void
    free_temp_sescb (ncxmod_temp_sescb_t *sescb)
{
    ncxmod_temp_filcb_t *filcb;
    int                  retval;

    while (!dlq_empty(&sescb->temp_filcbQ)) {
        filcb = (ncxmod_temp_filcb_t *)
            dlq_deque(&sescb->temp_filcbQ);
        free_temp_filcb(filcb);
    }

    if (sescb->source) {
        retval = rmdir((const char *)sescb->source);
        if (retval < 0) {
            log_error("\nError: could not delete temp directory '%s' (%s)\n",
                      sescb->source,
                      get_error_string(errno_to_status()));

        }
        m__free(sescb->source);
    }

    m__free(sescb);

}  /* free_temp_sescb */


/********************************************************************
* FUNCTION new_temp_progcb
* 
* Malloc a new temporary program instance control block
*
* RETURNS:
*  malloced and initialized struct
*********************************************************************/
static ncxmod_temp_progcb_t *
    new_temp_progcb (void)

{
    ncxmod_temp_progcb_t *newcb;

    newcb = m__getObj(ncxmod_temp_progcb_t);
    if (newcb == NULL) {
        return NULL;
    }

    memset(newcb, 0x0, sizeof(ncxmod_temp_progcb_t));
    dlq_createSQue(&newcb->temp_sescbQ);
    return newcb;

}  /* new_temp_progcb */


/********************************************************************
* FUNCTION free_temp_progcb
* 
* Clean and free a new temporary program instance control block
*
* INPUTS:
*    progcb == temp program instance control block to free
*
*********************************************************************/
static void
    free_temp_progcb (ncxmod_temp_progcb_t *progcb)
{
    ncxmod_temp_sescb_t *sescb;
    int                  retval;

    while (!dlq_empty(&progcb->temp_sescbQ)) {
        sescb = (ncxmod_temp_sescb_t *)
            dlq_deque(&progcb->temp_sescbQ);
        free_temp_sescb(sescb);
    }

    if (progcb->source) {
        retval = rmdir((const char *)progcb->source);
        if (retval < 0) {
            log_error("\nError: could not delete temp directory '%s' (%s)\n",
                      progcb->source,
                      get_error_string(errno_to_status()));

        }
        m__free(progcb->source);
    }

    m__free(progcb);

}  /* free_temp_progcb */


/********************************************************************
* FUNCTION make_search_result
*
* Make a ncxmod_search_result_t struct for the given module
*
* INPUTS:
*   mod == module to use
*
* RETURNS:
*   == malloced and filled in search result struct
*      MUST call ncxmod_free_search_result() if the
*      return value is non-NULL
*      CHECK result->res TO SEE IF MODULE WAS FOUND
*      A SEARCH RESULT WILL BE RETURNED IF A SEARCH WAS
*      ATTEMPTED, EVEN IF NOTHING FOUND
*   == NULL if any error preventing a search
*********************************************************************/
static ncxmod_search_result_t *
    make_search_result (ncx_module_t *mod)
{
    ncxmod_search_result_t   *searchresult;

    if (LOGDEBUG2) {
        log_debug2("\nFound %smodule"
                   "\n   name:      '%s'"
                   "\n   revision:  '%s':"
                   "\n   namespace: '%s'"
                   "\n   source:    '%s'",
                   (mod->ismod) ? "" : "sub",
                   (mod->name) ? mod->name : EMPTY_STRING,
                   (mod->version) ? mod->version : EMPTY_STRING,
                   (mod->ns) ? mod->ns : EMPTY_STRING,
                   (mod->source) ? mod->source : EMPTY_STRING);
    }

    searchresult = ncxmod_new_search_result_ex(mod);
    if (searchresult == NULL) {
        return NULL;
    }
    searchresult->res = NO_ERR;

    return searchresult;

}  /* make_search_result */


/********************************************************************
* FUNCTION search_subtree_callback
* 
* Handle the current filename in the subtree traversal
* Quick parse and generate a search result for the module
*
* INPUTS:
*   fullspec == absolute or relative path spec, with filename and ext.
*               this regular file exists, but has not been checked for
*               read access of 
*   cookie == resultQ to store malloced search result
*
* RETURNS:
*    status
*
*    Return fatal error to stop the traversal or NO_ERR to
*    keep the traversal going.  Do not return any warning or
*    recoverable error, just log and move on
*********************************************************************/
static status_t
    search_subtree_callback (const char *fullspec,
                             void *cookie)
{
    dlq_hdr_t               *resultQ;
    ncx_module_t            *retmod;
    yang_pcb_t              *pcb;
    ncxmod_search_result_t  *searchresult;
    status_t                 res;

    resultQ = (dlq_hdr_t *)cookie;

    if (resultQ == NULL) {
        return SET_ERROR(ERR_INTERNAL_VAL);
    }

    pcb = yang_new_pcb();
    if (pcb == NULL) {
        return ERR_INTERNAL_MEM;
    } 
    pcb->searchmode = TRUE;

    retmod = NULL;
    searchresult = NULL;

    res = try_load_module(pcb, YANG_PT_TOP, (const xmlChar *)fullspec, 
                          NULL, &retmod);

    if (res != NO_ERR || retmod == NULL) {
        if (LOGDEBUG2) {
            log_debug2("\nFind module '%s' failed (%s)",
                       fullspec,
                       get_error_string(res));
        }
    } else {
        searchresult = make_search_result(retmod);
        if (searchresult != NULL) {
            /* hand off malloced memory here */
            dlq_enque(searchresult, resultQ);
        } else {
            res = ERR_INTERNAL_MEM;
        }
    }

    if (pcb) {
        yang_free_pcb(pcb);
    }

    return res;

}  /* search_subtree_callback */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION ncxmod_init
* 
* Initialize the ncxmod module
*
* RETURNS:
*   status
*********************************************************************/
status_t
    ncxmod_init (void)
{
    status_t   res = NO_ERR;

#ifdef DEBUG
    if (ncxmod_init_done) {
        return SET_ERROR(ERR_INTERNAL_INIT_SEQ);
    }
#endif

    /* try to get the YUMA_HOME environment variable */
    ncxmod_yuma_home = (const xmlChar *)getenv(NCXMOD_HOME);

    ncxmod_yuma_home_cli = NULL;

    /* try to get the YUMA_INSTALL environment variable */
    ncxmod_env_install = (const xmlChar *)getenv(NCXMOD_INSTALL);

    /* try to get the user HOME environment variable */
    ncxmod_home = (const xmlChar *)getenv(USER_HOME);

    ncxmod_home_cli = NULL;

    /* try to get the module search path variable */
    ncxmod_mod_path = (const xmlChar *)getenv(NCXMOD_MODPATH);

    if (ncxmod_home != NULL) {
        ncxmod_yumadir_path = ncx_get_source(NCXMOD_YUMA_DIR, &res);
    } else {
        ncxmod_yumadir_path = xml_strdup(NCXMOD_TEMP_YUMA_DIR);
        if (ncxmod_yumadir_path == NULL) {
            res = ERR_INTERNAL_MEM;
        }
    }

    ncxmod_mod_path_cli = NULL;

    ncxmod_alt_path = NULL;

    /* try to get the data search path variable */
    ncxmod_data_path = (const xmlChar *)getenv(NCXMOD_DATAPATH);

    ncxmod_data_path_cli = NULL;

    /* try to get the script search path variable */
    ncxmod_run_path = (const xmlChar *)getenv(NCXMOD_RUNPATH);

    ncxmod_run_path_cli = NULL;

    ncxmod_subdirs = TRUE;

    ncxmod_init_done = TRUE;

    return res;

}  /* ncxmod_init */


/********************************************************************
* FUNCTION ncxmod_cleanup
* 
* Cleanup the ncxmod module
*
*********************************************************************/
void
    ncxmod_cleanup (void)
{
#ifdef DEBUG
    if (!ncxmod_init_done) {
        SET_ERROR(ERR_INTERNAL_INIT_SEQ);
        return;
    }
#endif
     
    ncxmod_yuma_home = NULL;
    ncxmod_env_install = NULL;
    ncxmod_home = NULL;
    ncxmod_mod_path = NULL;
    ncxmod_data_path = NULL;
    ncxmod_run_path = NULL;

    if (ncxmod_home_cli) {
        m__free(ncxmod_home_cli);
    }

    if (ncxmod_yuma_home_cli) {
        m__free(ncxmod_yuma_home_cli);
    }

    if (ncxmod_yumadir_path) {
        m__free(ncxmod_yumadir_path);
    }

    if (ncxmod_mod_path_cli) {
        m__free(ncxmod_mod_path_cli);
    }

    if (ncxmod_data_path_cli) {
        m__free(ncxmod_data_path_cli);
    }

    if (ncxmod_run_path_cli) {
        m__free(ncxmod_run_path_cli);
    }

    ncxmod_init_done = FALSE;
    
}  /* ncxmod_cleanup */


/********************************************************************
* FUNCTION ncxmod_load_module
*
* Determine the location of the specified module
* and then load it into the system, if not already loaded
*
* This is the only load module variant that checks if there
* are any errors recorded in the module or any of its dependencies
* !!! ONLY RETURNS TRUE IF MODULE AND ALL IMPORTS ARE LOADED OK !!!
*
* Module Search order:
*
* 1) YUMA_MODPATH environment var (or set by modpath CLI var)
* 2) current dir or absolute path
* 3) YUMA_HOME/modules directory
* 4) HOME/modules directory
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*   savedevQ == Q of ncx_save_deviations_t to use, if any
*   retmod == address of return module (may be NULL)
*
* OUTPUTS:
*   if non-NULL:
*    *retmod == pointer to requested module version
*
* RETURNS:
*   status
*********************************************************************/
status_t 
    ncxmod_load_module (const xmlChar *modname,
                        const xmlChar *revision,
                        dlq_hdr_t *savedevQ,
                        ncx_module_t **retmod)
{



    assert( modname && "modname is NULL!" );

    if (retmod) {
        *retmod = NULL;
    }

    status_t res = NO_ERR;
    yang_pcb_t *pcb = yang_new_pcb();
    if (!pcb) {
        res = ERR_INTERNAL_MEM;
    } else {
        pcb->revision = revision;
        pcb->savedevQ = savedevQ;

        res = try_load_module(pcb, YANG_PT_TOP, modname, revision, retmod);
    }

    if (LOGINFO && res != NO_ERR) {
        if (revision) {
            log_info("\nLoad module '%s', revision '%s' failed (%s)",
                     modname,
                     revision,
                     get_error_string(res));
        } else {
            log_info("\nLoad module '%s' failed (%s)",
                     modname,
                     get_error_string(res));
        }
    }

    if (pcb) {
        yang_free_pcb(pcb);
    }
    return res;

}  /* ncxmod_load_module */


/********************************************************************
* FUNCTION ncxmod_parse_module
*
* Determine the location of the specified module
* and then parse the file into an ncx_module_t,
* but do not load it into the module registry
*
* Used by yangcli to build per-session view of each
* module advertised by the NETCONF server
*
* Module Search order:
*
* 1) YUMA_MODPATH environment var (or set by modpath CLI var)
* 2) current dir or absolute path
* 3) YUMA_HOME/modules directory
* 4) HOME/modules directory
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*   savedevQ == Q of ncx_save_deviations_t to use, if any
*   retmod == address of return module
*
* OUTPUTS:
*    *retmod == pointer to requested module version
*               THIS IS A LIVE MALLOCED STRUCT THAT NEEDS
*               FREED LATER
*
* RETURNS:
*   status
*********************************************************************/
status_t 
    ncxmod_parse_module (const xmlChar *modname,
                         const xmlChar *revision,
                         dlq_hdr_t *savedevQ,
                         ncx_module_t **retmod)
{
    yang_pcb_t      *pcb;
    status_t         res;

#ifdef DEBUG
    if (!modname) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;

    pcb = yang_new_pcb();
    if (!pcb) {
        res = ERR_INTERNAL_MEM;
    } else {
        pcb->revision = revision;
        pcb->savedevQ = savedevQ;
        pcb->parsemode = TRUE;
        res = try_load_module(pcb, YANG_PT_TOP, modname, revision, retmod);
    }

    if (LOGINFO && res != NO_ERR) {
        if (revision) {
            log_info("\nLoad module '%s', revision '%s' failed (%s)",
                     modname,
                     revision,
                     get_error_string(res));
        } else {
            log_info("\nLoad module '%s' failed (%s)",
                     modname,
                     get_error_string(res));
        }
    }

    if (pcb) {
        yang_free_pcb(pcb);
    }
    return res;

}  /* ncxmod_parse_module */


/********************************************************************
* FUNCTION ncxmod_find_module
*
* Determine the location of the specified module
* and then fill out a search result struct
* DO NOT load it into the system
*
* Module Search order:
*
* 1) YUMA_MODPATH environment var (or set by modpath CLI var)
* 2) current dir or absolute path
* 3) YUMA_HOME/modules directory
* 4) HOME/modules directory
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*
* RETURNS:
*   == malloced and filled in search result struct
*      MUST call ncxmod_free_search_result() if the
*      return value is non-NULL
*      CHECK result->res TO SEE IF MODULE WAS FOUND
*      A SEARCH RESULT WILL BE RETURNED IF A SEARCH WAS
*      ATTEMPTED, EVEN IF NOTHING FOUND
*   == NULL if any error preventing a search
*********************************************************************/
ncxmod_search_result_t *
    ncxmod_find_module (const xmlChar *modname,
                        const xmlChar *revision)
{
    yang_pcb_t               *pcb;
    ncxmod_search_result_t   *searchresult;
    ncx_module_t             *retmod;
    status_t                  res;

#ifdef DEBUG
    if (!modname) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    pcb = yang_new_pcb();
    if (pcb == NULL) {
        return NULL;
    } 
    pcb->revision = revision;
    pcb->searchmode = TRUE;

    retmod = NULL;
    searchresult = NULL;

    res = try_load_module(pcb, YANG_PT_TOP, modname, revision, &retmod);

    if (res != NO_ERR || retmod == NULL) {
        if (LOGDEBUG2) {
            if (revision) {
                log_debug2("\nFind module '%s', revision '%s' failed (%s)",
                           modname,
                           revision,
                           get_error_string(res));
            } else {
                log_debug2("\nFind module '%s' failed (%s)",
                           modname,
                           get_error_string(res));
            }
        }
    } else {
        searchresult = make_search_result(retmod);
    }

    if (pcb) {
        yang_free_pcb(pcb);
    }
    
    return searchresult;

}  /* ncxmod_find_module */


/********************************************************************
* FUNCTION ncxmod_find_all_modules
*
* Determine the location of all possible YANG modules and submodules
* within the configured YUMA_MODPATH and default search path
* All files with .yang and .yin file extensions found in the
* search directories will be checked.
* 
* Does not cause modules to be fully parsed and registered.
* Quick parse only is done, and modules are discarded.
* Strings from the module are copied into the searchresult struct
*
* Module Search order:
*
* 1) YUMA_MODPATH environment var (or set by modpath CLI var)
* 2) HOME/modules directory
* 3) YUMA_HOME/modules directory
* 4) YUMA_INSTALL/modules directory
*
* INPUTS:
*   resultQ == address of Q to stor malloced search results
*
* OUTPUTS:
*  resultQ may have malloced ncxmod_zsearch_result_t structs
*  queued into it representing the modules found in the search
*
* RETURNS:
*  status
*********************************************************************/
status_t 
    ncxmod_find_all_modules (dlq_hdr_t *resultQ)
{
    xmlChar        *buff;
    uint32          bufflen;
    status_t        res;

#ifdef DEBUG
    if (!resultQ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;

    /* get a temp buffer to construct filespecs */
    bufflen = NCXMOD_MAX_FSPEC_LEN+1;
    buff = m__getMem(bufflen);
    if (!buff) {
        return ERR_INTERNAL_MEM;
    } else {
        *buff = 0;
    }

    /* 1) try YUMA_MODPATH environment variable if set */
    if (ncxmod_mod_path) {
        res = search_module_pathlist(ncxmod_mod_path,
                                     search_subtree_callback,
                                     resultQ);
    }

    /* 2) HOME/modules directory */
    if (res == NO_ERR && ncxmod_home) {
        res = search_module_path(ncxmod_home,
                                 buff,
                                 bufflen,
                                 search_subtree_callback,
                                 resultQ);
    }

    /* 3) YUMA_HOME/modules directory */
    if (res == NO_ERR && ncxmod_yuma_home) {
        res = search_module_path(ncxmod_yuma_home,
                                 buff,
                                 bufflen,
                                 search_subtree_callback,
                                 resultQ);
    }

    /* 4) YUMA_INSTALL/modules directory or default install path
     *    If this envvar is set then the default install path will not
     *    be tried
     */
    if (res == NO_ERR) {
        if (ncxmod_env_install) {
            res = search_module_path(ncxmod_env_install, 
                                     buff,
                                     bufflen,
                                     search_subtree_callback,
                                     resultQ);
        } else {
            res = search_module_path(NCXMOD_DEFAULT_INSTALL, 
                                     buff,
                                     bufflen,
                                     search_subtree_callback,
                                     resultQ);
        }
    }

    m__free(buff);

    return NO_ERR;  /* ignore any module errors found */
}  /* ncxmod_find_all_modules */


/********************************************************************
* FUNCTION ncxmod_load_deviation
*
* Determine the location of the specified module
* and then parse it in the special deviation mode
* and save any deviation statements in the Q that is provided
*
* Module Search order:
*
* 1) YUMA_MODPATH environment var (or set by modpath CLI var)
* 2) current dir or absolute path
* 3) YUMA_HOME/modules directory
* 4) HOME/modules directory
*
* INPUTS:
*   devname == deviation module name with 
*                   no path prefix or file extension
*   deviationQ == address of Q of ncx_save_deviations_t  structs
*                 to add any new entries 
*
* OUTPUTS:
*   if non-NULL:
*    deviationQ has malloced ncx_save_deviations_t structs added
*
* RETURNS:
*   status
*********************************************************************/
status_t 
    ncxmod_load_deviation (const xmlChar *deviname,
                           dlq_hdr_t *deviationQ)
{
    yang_pcb_t             *pcb;
    ncx_module_t           *retmod;
    ncx_save_deviations_t  *savedev;
    status_t                res;

#ifdef DEBUG
    if (!deviname || !deviationQ) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    res = NO_ERR;
    retmod = NULL;

    /* first check that this deviation has not aleady
     * been loaded and the user gave multiple leaf-list
     * values with the same name (may not get checked)
     */
    for (savedev = (ncx_save_deviations_t *)
             dlq_firstEntry(deviationQ);
         savedev != NULL;
         savedev = (ncx_save_deviations_t *)
             dlq_nextEntry(savedev)) {

        if (!xml_strcmp(deviname, savedev->devmodule)) {
            if (LOGDEBUG) {
                log_debug("\nSkipping duplicate deviation module '%s'",
                          deviname);
            }
            return NO_ERR;
        }
    }

    pcb = yang_new_pcb();
    if (!pcb) {
        res = ERR_INTERNAL_MEM;
    } else {
        pcb->deviationmode = TRUE;
        pcb->savedevQ = deviationQ;
        res = try_load_module(pcb, YANG_PT_TOP, deviname, NULL, &retmod);
    }

    if (res != NO_ERR) {
        log_error("\nError: Load deviation module '%s' failed (%s)\n",
                  deviname,
                  get_error_string(res));
    } else if (LOGDEBUG) {
        log_debug("\nLoad deviation module '%s' OK", deviname);
    }

    if (pcb) {
        yang_free_pcb(pcb);
    }
    return res;

}  /* ncxmod_load_deviation */


/********************************************************************
* FUNCTION ncxmod_load_imodule
*
* Determine the location of the specified module
* and then load it into the system, if not already loaded
*
* Called from an include or import or submodule
* Includes the YANG parser control block and new parser source type
*
* Module Search order:
*
* 1) YUMA_MODPATH environment var (or set by modpath CLI var)
* 2) current dir or absolute path
* 3) YUMA_HOME/modules directory
* 4) HOME/modules directory
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*   pcb == YANG parser control block
*   ptyp == YANG parser source type
*   parent == pointer to module being parsed if this is a
*     a request to parse a submodule; there is only 1 parent for 
*     all submodules, based on the value of belongs-to
*   retmod == address of return module
* OUTPUTS:
*  *retmod == pointer to found module (if NO_ERR)
*
* RETURNS:
*   status
*********************************************************************/
status_t 
    ncxmod_load_imodule (const xmlChar *modname,
                         const xmlChar *revision,
                         yang_pcb_t *pcb,
                         yang_parsetype_t ptyp,
                         ncx_module_t *parent,
                         ncx_module_t **retmod)
{
    yang_node_t     *node;
    const xmlChar   *savedrev;
    status_t         res;

#ifdef DEBUG
    if (!modname || !pcb) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* check if return module is requested */
    if (retmod != NULL) {
        *retmod = NULL;
    }

    /* see if [sub]module already tried and failed */
    node = yang_find_node(&pcb->failedQ, modname, revision);
    if (node) {
        return node->res;
    }

    savedrev = pcb->revision;
    pcb->revision = revision;
    pcb->parentparm = parent;

    res = try_load_module(pcb, ptyp, modname, revision, retmod);

    pcb->revision = savedrev;

    return res;

}  /* ncxmod_load_imodule */


/********************************************************************
* FUNCTION ncxmod_load_module_ex
*
* Determine the location of the specified module
* and then load it into the system, if not already loaded
* Return the PCB instead of deleting it
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*   with_submods == TRUE if YANG_PT_TOP mode should skip submodules
*                == FALSE if top-level mode should process sub-modules 
*   savetkc == TRUE if the parse chain should be saved (e.g., YIN)
*   keepmode == TRUE if pcb->top should be saved even if it
*               is already loaded;  FALSE will allow the mod
*               to be swapped out and the new copy deleted
*               yangdump sets this to true
*   docmode == TRUE if need to preserve strings for --format=html or yang
*           == FALSE if no need to preserve string token sequences
*   savedevQ == Q of ncx_save_deviations_t to use
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   pointer to malloced parser control block, or NULL of none
*********************************************************************/
yang_pcb_t *
    ncxmod_load_module_ex (const xmlChar *modname,
                           const xmlChar *revision,
                           boolean with_submods,
                           boolean savetkc,
                           boolean keepmode,
                           boolean docmode,
                           dlq_hdr_t *savedevQ,
                           status_t *res)
{
    yang_pcb_t     *pcb;

#ifdef DEBUG
    if (!modname || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    pcb = yang_new_pcb();
    if (!pcb) {
        *res = ERR_INTERNAL_MEM;
    } else {
        pcb->keepmode = keepmode;
        pcb->savedevQ = savedevQ;
        pcb->revision = revision;
        pcb->with_submods = with_submods;
        pcb->savetkc = savetkc;
        pcb->docmode = docmode;
        *res = try_load_module(pcb, YANG_PT_TOP, modname, revision, NULL);
    }

    return pcb;

}  /* ncxmod_load_module_ex */


/********************************************************************
* FUNCTION ncxmod_load_module_diff
*
* Determine the location of the specified module
* and then load it into the system, if not already loaded
* Return the PCB instead of deleting it
* !!Do not add definitions to the registry!!
*
* INPUTS:
*   modname == module name with no path prefix or file extension
*   revision == optional revision date of 'modname' to find
*   with_submods == TRUE if YANG_PT_TOP mode should skip submodules
*                == FALSE if top-level mode should process sub-modules 
*   modpath == module path to override the modpath CLI var or
*              the YUMA_MODPATH env var
*   savedevQ == Q of ncx_save_deviations_t to use
*   res == address of return status
*
* OUTPUTS:
*   *res == return status
*
* RETURNS:
*   pointer to malloced parser control block, or NULL of none
*********************************************************************/
yang_pcb_t *
    ncxmod_load_module_diff (const xmlChar *modname,
                             const xmlChar *revision,
                             boolean with_submods,
                             const xmlChar *modpath,
                             dlq_hdr_t *savedevQ,
                             status_t *res)
{
    yang_pcb_t    *pcb;

#ifdef DEBUG
    if (!modname || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    pcb = yang_new_pcb();
    if (!pcb) {
        *res = ERR_INTERNAL_MEM;
    } else {
        pcb->savedevQ = savedevQ;
        pcb->with_submods = with_submods;
        pcb->diffmode = TRUE;
        if (modpath) {
            ncxmod_set_altpath(modpath);
        }
        *res = try_load_module(pcb, YANG_PT_TOP, modname, revision, NULL);
        if (modpath) {
            ncxmod_clear_altpath();
        }
    }

    return pcb;

}  /* ncxmod_load_module_diff */


/********************************************************************
* FUNCTION ncxmod_find_data_file
*
* Determine the location of the specified data file
*
* Search order:
*
* 1) YUMA_DATAPATH environment var (or set by datapath CLI var)
* 2) current directory or absolute path
* 3) HOME/data directory
* 4) YUMA_HOME/data directory
* 5) HOME/.yuma/ directory
* 6a) YUMA_INSTALL/data directory OR
* 6b) /usr/share/yuma/data directory
* 7) /etc/yuma directory
*
* INPUTS:
*   fname == file name with extension
*            if the first char is '.' or '/', then an absolute
*            path is assumed, and the search path will not be tries
*   generrors == TRUE if error message should be generated
*                FALSE if no error message
*   res == address of status result
*
* OUTPUTS:
*   *res == status 
*
* RETURNS:
*   pointer to the malloced and initialized string containing
*   the complete filespec or NULL if not found
*   It must be freed after use!!!
*********************************************************************/
xmlChar *
    ncxmod_find_data_file (const xmlChar *fname,
                           boolean generrors,
                           status_t *res)
{
    xmlChar  *buff;
    uint32    flen, bufflen;

#ifdef DEBUG
    if (!fname || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    *res = NO_ERR;

    if (LOGDEBUG2) {
        log_debug2("\nNcxmod: Finding data file (%s)", 
                   fname);
    }

    flen = xml_strlen(fname);
    if (!flen || flen>NCX_MAX_NLEN) {
        *res = ERR_NCX_WRONG_LEN;
        return NULL;
    }

    /* get a buffer to construct filespacs */
    bufflen = NCXMOD_MAX_FSPEC_LEN+1;
    buff = m__getMem(bufflen);
    if (!buff) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    /* 1) try the YUMA_DATAPATH environment variable
     * only if it is not an absolute path string
     */
    if (ncxmod_data_path && (*fname != NCXMOD_PSCHAR)) {
        if (test_pathlist(ncxmod_data_path, buff, bufflen, fname, NULL)) {
            return buff;
        }
    }

    /* 2) current directory or absolute path */
    if (test_file(buff, bufflen, NULL, NULL, fname)) {
        return buff;
    }

    /* 3) HOME/data directory */
    if (ncxmod_home) {
        if (test_file(buff, bufflen, ncxmod_home, NCXMOD_DATA_DIR, fname)) {
            return buff;
        }
    }

    /* 4) YUMA_HOME/data directory */
    if (ncxmod_yuma_home) {
        if (test_file(buff, bufflen, ncxmod_yuma_home, NCXMOD_DATA_DIR, 
                      fname)) {
            return buff;
        }
    }

    /* 5) HOME/.yuma directory */
    if (ncxmod_home) {
        if (test_file(buff, bufflen, ncxmod_home, NCXMOD_YUMA_DIRNAME, fname)) {
            return buff;
        }
    }

    /* 6a) YUMA_INSTALL/data directory */
    if (ncxmod_env_install) {
        if (test_file(buff, bufflen, ncxmod_env_install, NCXMOD_DATA_DIR, 
                      fname)) {
            return buff;
        }
    } else if (test_file(buff, bufflen, NCXMOD_DEFAULT_INSTALL,
                         NCXMOD_DATA_DIR, fname)) {
        /* 6b) default YUMA_INSTALL data directory */
        return buff;
    }

    /* 7) default Yuma data directory */
    if (test_file(buff, bufflen, NCXMOD_ETC_DATA, NULL, fname)) {
        return buff;
    }

    if (generrors) {
        log_error("\nError: data file (%s) not found.\n", fname);
    }

    m__free(buff);
    *res = ERR_NCX_CFG_NOT_FOUND;
    return NULL;

}  /* ncxmod_find_data_file */


/********************************************************************
* FUNCTION ncxmod_find_sil_file
*
* Determine the location of the specified 
* server instrumentation library file
*
* Search order:
*
*  1) $YUMA_HOME/target/lib directory 
*  2) $YUMA_RUNPATH environment variable
*  3) $YUMA_INSTALL/lib
*  4) $YUMA_INSTALL/lib/yuma
*  5) /usr/lib64/yuma directory (LIB64 only)
*  5 or 6) /usr/lib/yuma directory
*
* INPUTS:
*   fname == SIL file name with extension
*   generrors == TRUE if error message should be generated
*                FALSE if no error message
*   res == address of status result
*
* OUTPUTS:
*   *res == status 
*
* RETURNS:
*   pointer to the malloced and initialized string containing
*   the complete filespec or NULL if not found
*   It must be freed after use!!!
*********************************************************************/
xmlChar *
    ncxmod_find_sil_file (const xmlChar *fname,
                          boolean generrors,
                          status_t *res)
{
    xmlChar  *buff;
    uint32    flen, bufflen;

#ifdef DEBUG
    if (!fname || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    *res = NO_ERR;

    if (LOGDEBUG2) {
        log_debug2("\nNcxmod: Finding SIL file (%s)", 
                   fname);
    }

    flen = xml_strlen(fname);
    if (!flen || flen>NCX_MAX_NLEN) {
        *res = ERR_NCX_WRONG_LEN;
        return NULL;
    }

    /* get a buffer to construct filespacs */
    bufflen = NCXMOD_MAX_FSPEC_LEN+1;
    buff = m__getMem(bufflen);
    if (!buff) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    /* 1) YUMA_HOME/target/lib directory */
    if (ncxmod_yuma_home) {
        if (test_file(buff, 
                      bufflen, 
                      ncxmod_yuma_home,
                      (const xmlChar *)"target/lib/",
                      fname)) {
            return buff;
        }
    }

    /* 2) try the YUMA_RUNPATH environment variable */
    if (ncxmod_run_path) {
        if (test_pathlist(ncxmod_run_path, 
                          buff, 
                          bufflen, 
                          fname, 
                          NULL)) {
            return buff;
        }
    }

    /* 3) YUMA_INSTALL/lib directory */
    if (ncxmod_env_install) {
        if (test_file(buff, 
                      bufflen, 
                      ncxmod_env_install,
                      (const xmlChar *)"lib",
                      fname)) {
            return buff;
        }
    }

    /* 4) YUMA_INSTALL/lib/yuma directory */
    if (ncxmod_env_install) {
        if (test_file(buff, 
                      bufflen, 
                      ncxmod_env_install,
                      (const xmlChar *)"lib/yuma",
                      fname)) {
            return buff;
        }
    }

#ifdef LIB64
    /* 5) /usr/lib64/yuma directory */
    if (test_file(buff, 
                  bufflen, 
                  NCXMOD_DEFAULT_YUMALIB64,
                  NULL,
                  fname)) {
        return buff;
    }
#endif

    /* 5 or 6) /usr/lib/yuma directory */
    if (test_file(buff, 
                  bufflen, 
                  NCXMOD_DEFAULT_YUMALIB,
                  NULL,
                  fname)) {
        return buff;
    }

    if (generrors) {
        log_error("\nError: SIL file (%s) not found.\n", fname);
    }

    m__free(buff);
    *res = ERR_NCX_MOD_NOT_FOUND;
    return NULL;

}  /* ncxmod_find_sil_file */


/********************************************************************
* FUNCTION ncxmod_make_data_filespec
*
* Determine a suitable path location for the specified data file name
*
* Search order:
*
* 1) YUMA_DATAPATH environment var (or set by datapath CLI var)
* 2) HOME/data directory
* 3) YUMA_HOME/data directory
* 4) HOME/.yuma directory
* 5) YUMA_INSTALL/data directory
* 6) current directory
*
* INPUTS:
*   fname == file name with extension
*            if the first char is '.' or '/', then an absolute
*            path is assumed, and the search path will not be tries
*   res == address of status result
*
* OUTPUTS:
*   *res == status 
*
* RETURNS:
*   pointer to the malloced and initialized string containing
*   the complete filespec or NULL if no suitable location
*   for the datafile is found
*   It must be freed after use!!!
*********************************************************************/
xmlChar *
    ncxmod_make_data_filespec (const xmlChar *fname,
                               status_t *res)
{
    xmlChar  *buff;
    uint32    flen, pathlen, bufflen;

#ifdef DEBUG
    if (!fname || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    *res = NO_ERR;

    flen = xml_strlen(fname);
    if (!flen || flen > NCX_MAX_NLEN) {
        *res = ERR_NCX_WRONG_LEN;
        return NULL;
    }

    /* get a buffer to construct filespecs */
    bufflen = NCXMOD_MAX_FSPEC_LEN+1;
    buff = m__getMem(bufflen);
    if (!buff) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    /* 1) try the YUMA_DATAPATH environment variable */
    if (ncxmod_data_path) {
        if (test_pathlist_make(ncxmod_data_path, 
                               buff, 
                               bufflen)) {
            pathlen = xml_strlen(buff);
            if ((pathlen + flen) < bufflen) {
                xml_strcat(buff, fname);
            } else {
                *res = ERR_BUFF_OVFL;
                m__free(buff);
                return NULL;
            }
        }
    }

    /* 2) HOME/data directory */
    if (ncxmod_home) {
        if (test_file_make(buff, bufflen, ncxmod_home, NCXMOD_DATA_DIR, 
                           fname) == NO_ERR) {
            return buff;
        }
    }

    /* 3) YUMA_HOME/data directory */
    if (ncxmod_yuma_home) {
        if (test_file_make(buff, bufflen, ncxmod_yuma_home, NCXMOD_DATA_DIR, 
                           fname) == NO_ERR) {
            return buff;
        }
    }

    /* 4) HOME/.yuma directory */
    if (ncxmod_home) {
        if (test_file_make(buff, bufflen, ncxmod_home, NCXMOD_YUMA_DIRNAME, 
                           fname) == NO_ERR) {
            return buff;
        }
    }

    /* 5) YUMA_INSTALL/data directory */
    if (ncxmod_env_install) {
        if (test_file_make(buff, bufflen, ncxmod_env_install, NCXMOD_DATA_DIR, 
                           fname) == NO_ERR) {
            return buff;
        }
    } else {
        if (test_file_make(buff, bufflen, NCXMOD_DEFAULT_INSTALL,
                           NCXMOD_DATA_DIR, fname) == NO_ERR) {
            return buff;
        }
    }

    /* 6) current directory */
    if (test_file_make(buff, bufflen, NULL, NULL, fname) == NO_ERR) {
        return buff;
    }

    m__free(buff);
    *res = ERR_NCX_MOD_NOT_FOUND;
    return NULL;

}  /* ncxmod_make_data_filespec */


/********************************************************************
* FUNCTION ncxmod_make_data_filespec_from_src
*
* Determine the directory path portion of the specified
* source_url and change the filename to the specified filename
* in a new copy of the complete filespec
*
* INPUTS:
*   srcspec == source filespec to use
*   fname == file name with extension
*            if the first char is '.' or '/', then an absolute
*            path is assumed, and the search path will not be tries
*   res == address of status result
*
* OUTPUTS:
*   *res == status 
*
* RETURNS:
*   pointer to the malloced and initialized string containing
*   the complete filespec or NULL if some error occurred
*********************************************************************/
xmlChar *
    ncxmod_make_data_filespec_from_src (const xmlChar *srcspec,
                                        const xmlChar *fname,
                                        status_t *res)
{
    const xmlChar  *str;
    xmlChar        *buff, *p;
    uint32          flen, pathlen, copylen, bufflen;

#ifdef DEBUG
    if (!srcspec || !fname || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    *res = NO_ERR;

    pathlen = xml_strlen(srcspec);
    if (pathlen == 0) {
        *res = ERR_NCX_WRONG_LEN;
        return NULL;
    }

    flen = xml_strlen(fname);
    if (!flen || flen > NCX_MAX_NLEN) {
        *res = ERR_NCX_WRONG_LEN;
        return NULL;
    }

    bufflen = flen+1;
    copylen = 0;

    /* find the end of the path portion of the srcspec */
    str = &srcspec[pathlen-1];
    while (str >= srcspec && *str != NCXMOD_PSCHAR) {
        str--;
    }
    if (*str == NCXMOD_PSCHAR) {
        copylen = (uint32)(str - srcspec) + 1;
        bufflen += copylen;
    }

    /* get a buffer to construct the filespec */
    buff = m__getMem(bufflen);
    if (!buff) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    p = buff;
    if (copylen) {
        p += xml_strncpy(p, srcspec, copylen);
    }
    xml_strcpy(p, fname);

    return buff;

}  /* ncxmod_make_data_filespec_from_src */


/********************************************************************
* FUNCTION ncxmod_find_script_file
*
* Determine the location of the specified script file
*
* Search order:
*
* 1) current directory or absolute path
* 2) YUMA_RUNPATH environment var (or set by runpath CLI var)
* 3) HOME/scripts directory
* 4) YUMA_HOME/scripts directory
* 5) YUMA_INSTALL/scripts directory
*
* INPUTS:
*   fname == file name with extension
*            if the first char is '.' or '/', then an absolute
*            path is assumed, and the search path will not be tries
*   res == address of status result
*
* OUTPUTS:
*   *res == status 
*
* RETURNS:
*   pointer to the malloced and initialized string containing
*   the complete filespec or NULL if not found
*   It must be freed after use!!!
*********************************************************************/
xmlChar *
    ncxmod_find_script_file (const xmlChar *fname,
                             status_t *res)
{
    xmlChar  *buff;
    uint32    flen, bufflen;

#ifdef DEBUG
    if (!fname || !res) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    *res = NO_ERR;

    if (LOGDEBUG2) {
        log_debug2("\nNcxmod: Finding script file (%s)", 
                   fname);
    }

    flen = xml_strlen(fname);
    if (!flen || flen>NCX_MAX_NLEN) {
        *res = ERR_NCX_WRONG_LEN;
        return NULL;
    }

    /* get a buffer to construct filespacs */
    bufflen = NCXMOD_MAX_FSPEC_LEN+1;
    buff = m__getMem(bufflen);
    if (!buff) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    /* 1) try 'fname' as a current directory or absolute path */
    if (test_file(buff, bufflen, NULL, NULL, fname)) {
        return buff;
    }

    /* look for the script file  'fname'
     * 2) check YUMA_RUNPATH env-var or runpath CLI param 
     */
    if (ncxmod_run_path) {
        if (test_pathlist(ncxmod_run_path, buff, bufflen, fname, NULL)) {
            return buff;
        }
    }

    /* 3) try HOME/scripts/fname */
    if (ncxmod_home) {
        if (test_file(buff, bufflen, ncxmod_home, NCXMOD_SCRIPT_DIR, fname)) {
            return buff;
        }
    }

    /* 4) try YUMA_HOME/scripts/fname */
    if (ncxmod_yuma_home) {
        if (test_file(buff, bufflen, ncxmod_yuma_home, NCXMOD_SCRIPT_DIR, 
                      fname)) {
            return buff;
        }
    }

    /* 5) YUMA_INSTALL/scripts directory or default install path
     *    If this envvar is set then the default install path will not
     *    be tried
     */
    if (ncxmod_env_install) {
        if (test_file(buff, bufflen, ncxmod_env_install, NCXMOD_SCRIPT_DIR, 
                      fname)) {
            return buff;
        }
    } else {
        if (test_file(buff, bufflen, NCXMOD_DEFAULT_INSTALL, NCXMOD_SCRIPT_DIR, 
                      fname)) {
            return buff;
        }
    }

    log_info("\nError: script file (%s) not found.", fname);

    m__free(buff);
    *res = ERR_NCX_MISSING_FILE;
    return NULL;

}  /* ncxmod_find_script_file */


/********************************************************************
* FUNCTION ncxmod_set_home
* 
*   Override the HOME env var with the home CLI var
*
* THIS MAY GET SET DURING BOOTSTRAP SO SET_ERROR NOT CALLED !!!
* MALLOC FAILED IGNORED!!!
*
* INPUTS:
*   home == new HOME value
*        == NULL or empty string to disable
*********************************************************************/
void
    ncxmod_set_home (const xmlChar *home)
{
    xmlChar    *savehome = ncxmod_home_cli;
    xmlChar    *savepath = ncxmod_yumadir_path;
    status_t    res = NO_ERR;

    if (home && *home) {
        if (*home == '/') {
            ncxmod_home_cli = xml_strdup(home);
            if (ncxmod_home_cli == NULL) {
                res = ERR_INTERNAL_MEM;
            }
        } else {
            ncxmod_home_cli = ncx_get_source(home, &res);            
        }
    } else {
        log_error("\nError: cannot set 'home' to empty string\n");
        return;
    }

    if (ncxmod_home_cli == NULL) {
        log_error("\nError: set home to '%s' failed (%s)\n", 
                  home, get_error_string(res));
        ncxmod_home_cli = savehome;
        return;
    }
        
    ncxmod_home = ncxmod_home_cli;
    if (savehome) {
        m__free(savehome);
    }

    /* reset the yumadir path if needed */
    if (ncxmod_home != NULL) {
        ncxmod_yumadir_path = ncx_get_source(NCXMOD_YUMA_DIR, &res);
    } else {
        ncxmod_yumadir_path = xml_strdup(NCXMOD_TEMP_YUMA_DIR);
        if (ncxmod_yumadir_path == NULL) {
            res = ERR_INTERNAL_MEM;
        }
    }
    if (ncxmod_yumadir_path == NULL) {
        log_error("\nError: set yumadir_path to '%s' failed (%s)\n", 
                  home, get_error_string(res));
        ncxmod_yumadir_path = savepath;
        return;
    }
    if (savepath) {
        m__free(savepath);
    }
    
}  /* ncxmod_set_home */


/********************************************************************
* FUNCTION ncxmod_get_home
*
*  Get the HOME or --home parameter value,
*  whichever is in effect, if any
*
* RETURNS:
*   const point to the home variable, or NULL if not set
*********************************************************************/
const xmlChar *
    ncxmod_get_home (void)
{
    return ncxmod_home;

}  /* ncxmod_get_home */


/********************************************************************
* FUNCTION ncxmod_set_yuma_home
* 
*   Override the YUMA_HOME env var with the yuma-home CLI var
*
* THIS MAY GET SET DURING BOOTSTRAP SO SET_ERROR NOT CALLED !!!
* MALLOC FAILED IGNORED!!!
*
* INPUTS:
*   yumahome == new YUMA_HOME value
*********************************************************************/
void
    ncxmod_set_yuma_home (const xmlChar *yumahome)
{
    xmlChar *savehome = ncxmod_yuma_home_cli;
    status_t res = NO_ERR;

    if (yumahome && *yumahome) {
        if (*yumahome == '/') {
            ncxmod_yuma_home_cli = xml_strdup(yumahome);
            if (ncxmod_yuma_home_cli == NULL) {
                res = ERR_INTERNAL_MEM;
            }
        } else {
            ncxmod_yuma_home_cli = ncx_get_source(yumahome, &res);
        }
        if (ncxmod_yuma_home_cli == NULL) {
            log_error("\nError: set yuma home to '%s' failed (%s)",
                      yumahome, get_error_string(res));
            return;
        }

        ncxmod_yuma_home = ncxmod_yuma_home_cli;
        if (savehome) {
            m__free(savehome);
        }
    } else {
        log_error("\nError: cannot set yuma home to empty string\n");
        return;
    }
    
}  /* ncxmod_set_yuma_home */


/********************************************************************
* FUNCTION ncxmod_get_yuma_home
*
*  Get the YUMA_HOME or --yuma-home parameter value,
*  whichever is in effect, if any
*
* RETURNS:
*   const point to the yuma_home variable, or NULL if not set
*********************************************************************/
const xmlChar *
    ncxmod_get_yuma_home (void)
{
    return ncxmod_yuma_home;

}  /* ncxmod_get_yuma_home */


/********************************************************************
* FUNCTION ncxmod_get_yuma_install
*
*  Get the YUMA_INSTALL or default install parameter value,
*  whichever is in effect
*
* RETURNS:
*   const point to the YUMA_INSTALL value
*********************************************************************/
const xmlChar *
    ncxmod_get_yuma_install (void)
{
    if (ncxmod_env_install) {
        return ncxmod_env_install;
    } else {
        return NCXMOD_DEFAULT_INSTALL;
    }

}  /* ncxmod_get_yuma_install */


/********************************************************************
* FUNCTION ncxmod_set_modpath
* 
*   Override the YUMA_MODPATH env var with the modpath CLI var
*
* THIS MAY GET SET DURING BOOTSTRAP SO SET_ERROR NOT CALLED !!!
* MALLOC FAILED IGNORED!!!
*
* INPUTS:
*   modpath == new YUMA_MODPATH value
*           == NULL or empty string to disable
*********************************************************************/
void
    ncxmod_set_modpath (const xmlChar *modpath)
{
    if (ncxmod_mod_path_cli) {
        m__free(ncxmod_mod_path_cli);
        ncxmod_mod_path_cli = NULL;
    }

    if (modpath && *modpath) {
        /* ignoring possible malloc failed!! */
        ncxmod_mod_path_cli = xml_strdup(modpath);
    }
    ncxmod_mod_path = ncxmod_mod_path_cli;
    
}  /* ncxmod_set_modpath */


/********************************************************************
* FUNCTION ncxmod_set_datapath
* 
*   Override the YUMA_DATAPATH env var with the datapath CLI var
*
* INPUTS:
*   datapath == new YUMA_DATAPATH value
*           == NULL or empty string to disable
*
*********************************************************************/
void
    ncxmod_set_datapath (const xmlChar *datapath)
{
    if (ncxmod_data_path_cli) {
        m__free(ncxmod_data_path_cli);
        ncxmod_data_path_cli = NULL;
    }

    if (datapath && *datapath) {
        /* ignoring possible malloc failed!! */
        ncxmod_data_path_cli = xml_strdup(datapath);
    }
    ncxmod_data_path = ncxmod_data_path_cli;

}  /* ncxmod_set_datapath */


/********************************************************************
* FUNCTION ncxmod_set_runpath
* 
*   Override the YUMA_RUNPATH env var with the runpath CLI var
*
* INPUTS:
*   datapath == new YUMA_RUNPATH value
*           == NULL or empty string to disable
*
*********************************************************************/
void
    ncxmod_set_runpath (const xmlChar *runpath)
{
    if (ncxmod_run_path_cli) {
        m__free(ncxmod_run_path_cli);
        ncxmod_run_path_cli = NULL;
    }

    if (runpath && *runpath) {
        /* ignoring possible malloc failed!! */
        ncxmod_run_path_cli = xml_strdup(runpath);
    }
    ncxmod_run_path = ncxmod_run_path_cli;
    
}  /* ncxmod_set_runpath */


/********************************************************************
* FUNCTION ncxmod_set_subdirs
* 
*   Set the subdirs flag to FALSE if the no-subdirs CLI param is set
*
* INPUTS:
*  usesubdirs == TRUE if subdirs searchs should be done
*             == FALSE if subdir searches should not be done
*********************************************************************/
void
    ncxmod_set_subdirs (boolean usesubdirs)
{
    ncxmod_subdirs = usesubdirs;
    
}  /* ncxmod_set_subdirs */


/********************************************************************
* FUNCTION ncxmod_get_yumadir
* 
*   Get the yuma directory being used
*
* RETURNS:
*   pointer to the yuma dir string
*********************************************************************/
const xmlChar *
    ncxmod_get_yumadir (void)
{
    return ncxmod_yumadir_path;
    
}  /* ncxmod_get_yumadir */


/********************************************************************
* FUNCTION ncxmod_process_subtree
*
* Search the entire specified subtree, looking for YANG
* modules.  Invoke the callback function for each module
* file found
*
* INPUTS:
*    startspec == absolute or relative pathspec to start
*            the search.  If this is not a valid pathname,
*            processing will exit immediately.
*    callback == address of the ncxmod_callback_fn_t function
*         to use for this traveral
*    cookie == cookie to pass to each invocation of the callback
*
* RETURNS:
*    NO_ERR if file found okay, full filespec in the 'buff' variable
*    OR some error if not found or buffer overflow
*********************************************************************/
status_t
    ncxmod_process_subtree (const char *startspec, 
                            ncxmod_callback_fn_t callback,
                            void *cookie)
{
    xmlChar       *sourcespec;
    char          *buff;
    DIR           *dp;
    uint32         bufflen;
    status_t       res;

#ifdef DEBUG
    if (!startspec || !callback) {
        return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    if (strlen(startspec) >= NCXMOD_MAX_FSPEC_LEN) {
        log_error("\nError: startspec too long '%s'\n", startspec);
        return ERR_BUFF_OVFL;
    }

    res = NO_ERR;
    sourcespec = ncx_get_source((const xmlChar *)startspec, &res);
    if (!sourcespec) {
        return res;
    }

    dp = opendir((const char *)sourcespec);
    if (!dp) {
        if (LOGDEBUG) {
            log_debug("\nncxmod: could not open directory '%s'\n", 
		      startspec);
	}
        m__free(sourcespec);
        return NO_ERR;
    } else {
        (void)closedir(dp);
    }

    bufflen = NCXMOD_MAX_FSPEC_LEN+1;
    buff = m__getMem(bufflen);
    if (!buff) {
        m__free(sourcespec);
        return ERR_INTERNAL_MEM;
    }

    strncpy(buff, (const char *)sourcespec, bufflen);
    res = process_subtree(buff, bufflen, callback, cookie);

    m__free(sourcespec);
    m__free(buff);
    return res;

}  /* ncxmod_process_subtree */


/********************************************************************
* FUNCTION ncxmod_test_subdir
*
* Check if the specified string is a directory
*
* INPUTS:
*    dirspec == string to check as a directory spec
*
* RETURNS:
*    TRUE if the string is a directory spec that this user
*       is allowed to open
*    FALSE otherwise
*********************************************************************/
boolean
    ncxmod_test_subdir (const xmlChar *dirspec)
{
    DIR           *dp;

    /* try to open the buffer spec as a directory */
    dp = opendir((const char *)dirspec);
    if (!dp) {
        return FALSE;
    } else {
        (void)closedir(dp);
        return TRUE;
    }
    /*NOTREACHED*/

}  /* ncxmod_test_subdir */


/********************************************************************
* FUNCTION ncxmod_get_userhome
*
* Get the user home dir from the passwd file
*
* INPUTS:
*    user == user name string (may not be zero-terminiated)
*    userlen == length of user
*
* RETURNS:
*    const pointer to the user home directory string
*********************************************************************/
const xmlChar *
    ncxmod_get_userhome (const xmlChar *user,
                  uint32 userlen)
{
    struct passwd  *pw;
    char            buff[NCX_MAX_USERNAME_LEN+1];

    /* only support user names up to N chars in length */
    if (userlen > NCX_MAX_USERNAME_LEN) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    if (!user) {
        return (const xmlChar *)ncxmod_home;
    }

    strncpy(buff, (const char *)user, userlen);
    buff[userlen] = 0;
    pw = getpwnam(buff);
    if (!pw) {
        return NULL;
    }

    return (const xmlChar *)pw->pw_dir;

}  /* ncxmod_get_userhome */


/********************************************************************
* FUNCTION ncxmod_get_envvar
*
* Get the specified shell environment variable
*
* INPUTS:
*    name == name of the environment variable (may not be zero-terminiated)
*    namelen == length of name string
*
* RETURNS:
*    const pointer to the specified environment variable value
*********************************************************************/
const xmlChar *
    ncxmod_get_envvar (const xmlChar *name,
                       uint32 namelen)
{
    char            buff[NCX_MAX_USERNAME_LEN+1];

#ifdef DEBUG
    if (!name) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    /* only support user names up to N chars in length */
    if (namelen > NCX_MAX_USERNAME_LEN) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }

    strncpy(buff, (const char *)name, namelen);
    buff[namelen] = 0;
    return (const xmlChar *)getenv(buff);

}  /* ncxmod_get_envvar */


/********************************************************************
* FUNCTION ncxmod_set_altpath
*
* Set the alternate path that should be used first (for yangdiff)
*
* INPUTS:
*    altpath == full path string to use
*               must be static 
*               a const back-pointer is kept, not a copy
*
*********************************************************************/
void
    ncxmod_set_altpath (const xmlChar *altpath)
{
#ifdef DEBUG
    if (!altpath) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    ncxmod_alt_path = altpath;

}  /* ncxmod_set_altpath */


/********************************************************************
* FUNCTION ncxmod_clear_altpath
*
* Clear the alternate path so none is used (for yangdiff)
*
*********************************************************************/
void
    ncxmod_clear_altpath (void)
{
    ncxmod_alt_path = NULL;

}  /* ncxmod_clear_altpath */


/********************************************************************
* FUNCTION ncxmod_list_data_files
*
* List the available data files found in the data search parh
*
* Search order:
*
* 1) current directory or absolute path
* 2) YUMA_DATAPATH environment var (or set by datapath CLI var)
* 3) HOME/data directory
* 4) YUMA_HOME/data directory
* 5) YUMA_INSTALL/data directory
*
* INPUTS:
*   helpmode == BRIEF, NORMAL or FULL 
*   logstdout == TRUE to use log_stdout
*                FALSE to use log_write
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t
    ncxmod_list_data_files (help_mode_t helpmode,
                            boolean logstdout)
{
    xmlChar     *buff, *p;
    uint32       bufflen, pathlen;
    status_t    res;


    /* get a buffer to construct filespacs */
    bufflen = NCXMOD_MAX_FSPEC_LEN+1;
    buff = m__getMem(bufflen);
    if (!buff) {
        return ERR_INTERNAL_MEM;
    }

    /* 1) current directory */
    xml_strcpy(buff, (const xmlChar *)"./");
    res = list_subdirs(buff, 
                       bufflen,
                       SEARCH_TYPE_DATA,
                       helpmode,
                       logstdout,
                       FALSE);
    if (res != NO_ERR) {
        m__free(buff);
        return res;
    }

    /* 2) try the YUMA_DATAPATH environment variable */
    if (ncxmod_data_path) {
        res = list_pathlist(ncxmod_data_path, 
                            buff,
                            bufflen,
                            SEARCH_TYPE_DATA,
                            helpmode,
                            logstdout);
        if (res != NO_ERR) {
            m__free(buff);
            return res;
        }
    }

    /* 3) HOME/data directory */
    if (ncxmod_home) {
        pathlen = xml_strlen(ncxmod_home);
        if (pathlen + 6 < bufflen) {
            p = buff;
            p += xml_strcpy(p, ncxmod_home);
            *p++ = NCXMOD_PSCHAR;
            p += xml_strcpy(p, (const xmlChar *)"data");
            *p++ = NCXMOD_PSCHAR;
            *p = '\0';

            res = list_subdirs(buff, 
                               bufflen, 
                               SEARCH_TYPE_DATA,
                               helpmode,
                               logstdout,
                               TRUE);
            if (res != NO_ERR) {
                m__free(buff);
                return res;
            }
        }
    }

    /* 4) YUMA_HOME/data directory */
    if (ncxmod_yuma_home) {
        pathlen = xml_strlen(ncxmod_yuma_home);;
        if (pathlen + 6 < bufflen) {
            p = buff;
            p += xml_strcpy(p, ncxmod_yuma_home);
            *p++ = NCXMOD_PSCHAR;
            p += xml_strcpy(p, (const xmlChar *)"data");
            *p++ = NCXMOD_PSCHAR;
            *p = '\0';

            res = list_subdirs(buff, 
                               bufflen, 
                               SEARCH_TYPE_DATA,
                               helpmode,
                               logstdout,
                               TRUE);
            if (res != NO_ERR) {
                m__free(buff);
                return res;
            }
        }
    }

    /* 5) YUMA_INSTALL/data directory */
    if (ncxmod_env_install) {
        pathlen = xml_strlen(ncxmod_env_install);;
        if (pathlen + 6 < bufflen) {
            p = buff;
            p += xml_strcpy(p, ncxmod_env_install);
            *p++ = NCXMOD_PSCHAR;
            p += xml_strcpy(p, (const xmlChar *)"data");
            *p++ = NCXMOD_PSCHAR;
            *p = '\0';

            res = list_subdirs(buff, 
                               bufflen, 
                               SEARCH_TYPE_DATA,
                               helpmode,
                               logstdout,
                               TRUE);
            if (res != NO_ERR) {
                m__free(buff);
                return res;
            }
        }
    }

    if (logstdout) {
        log_stdout("\n");
    } else {
        log_write("\n");
    }

    m__free(buff);
    return NO_ERR;

}  /* ncxmod_list_data_files */


/********************************************************************
* FUNCTION ncxmod_list_script_files
*
* List the available script files found in the 'run' search parh
*
* Search order:
*
* 1) current directory or absolute path
* 2) YUMA_RUNPATH environment var (or set by datapath CLI var)
* 3) HOME/scripts directory
* 4) YUMA_HOME/scripts directory
* 5) YUMA_INSTALL/scripts directory
*
* INPUTS:
*   helpmode == BRIEF, NORMAL or FULL 
*   logstdout == TRUE to use log_stdout
*                FALSE to use log_write
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t
    ncxmod_list_script_files (help_mode_t helpmode,
                              boolean logstdout)
{
    xmlChar     *buff, *p;
    uint32       bufflen, pathlen;
    status_t    res;


    /* get a buffer to construct filespacs */
    bufflen = NCXMOD_MAX_FSPEC_LEN+1;
    buff = m__getMem(bufflen);
    if (!buff) {
        return ERR_INTERNAL_MEM;
    }

    /* 1) current directory */
    xml_strcpy(buff, (const xmlChar *)"./");
    res = list_subdirs(buff, 
                       bufflen,
                       SEARCH_TYPE_SCRIPT,
                       helpmode,
                       logstdout,
                       FALSE);
    if (res != NO_ERR) {
        m__free(buff);
        return res;
    }

    /* 2) try the NCX_RUNPATH environment variable */
    if (ncxmod_run_path) {
        res = list_pathlist(ncxmod_run_path, 
                            buff,
                            bufflen,
                            SEARCH_TYPE_SCRIPT,
                            helpmode,
                            logstdout);
        if (res != NO_ERR) {
            m__free(buff);
            return res;
        }
    }

    /* 3) HOME/scripts directory */
    if (ncxmod_home) {
        pathlen = xml_strlen(ncxmod_home);
        if (pathlen + 9 < bufflen) {
            p = buff;
            p += xml_strcpy(p, ncxmod_home);
            *p++ = NCXMOD_PSCHAR;
            p += xml_strcpy(p, (const xmlChar *)"scripts");
            *p++ = NCXMOD_PSCHAR;
            *p = '\0';

            res = list_subdirs(buff, 
                               bufflen, 
                               SEARCH_TYPE_SCRIPT,
                               helpmode,
                               logstdout,
                               TRUE);
            if (res != NO_ERR) {
                m__free(buff);
                return res;
            }
        }
    }

    /* 4) YUMA_HOME/scripts directory */
    if (ncxmod_yuma_home) {
        pathlen = xml_strlen(ncxmod_yuma_home);;
        if (pathlen + 9 < bufflen) {
            p = buff;
            p += xml_strcpy(p, ncxmod_yuma_home);
            *p++ = NCXMOD_PSCHAR;
            p += xml_strcpy(p, (const xmlChar *)"scripts");
            *p++ = NCXMOD_PSCHAR;
            *p = '\0';

            res = list_subdirs(buff, 
                               bufflen, 
                               SEARCH_TYPE_SCRIPT,
                               helpmode,
                               logstdout,
                               TRUE);
            if (res != NO_ERR) {
                m__free(buff);
                return res;
            }
        }
    }

    /* 5) YUMA_INSTALL/scripts directory */
    if (ncxmod_env_install) {
        pathlen = xml_strlen(ncxmod_env_install);;
        if (pathlen + 9 < bufflen) {
            p = buff;
            p += xml_strcpy(p, ncxmod_env_install);
            *p++ = NCXMOD_PSCHAR;
            p += xml_strcpy(p, (const xmlChar *)"scripts");
            *p++ = NCXMOD_PSCHAR;
            *p = '\0';

            res = list_subdirs(buff, 
                               bufflen, 
                               SEARCH_TYPE_SCRIPT,
                               helpmode,
                               logstdout,
                               TRUE);
            if (res != NO_ERR) {
                m__free(buff);
                return res;
            }
        }
    }

    if (logstdout) {
        log_stdout("\n");
    } else {
        log_write("\n");
    }

    m__free(buff);
    return NO_ERR;

}  /* ncxmod_list_script_files */


/********************************************************************
* FUNCTION ncxmod_list_yang_files
*
* List the available YANG files found in the 'mod' search parh
*
* Search order:
*
* 1) current directory or absolute path
* 2) YUMA_MODPATH environment var (or set by datapath CLI var)
* 3) HOME/modules directory
* 4) YUMA_HOME/modules directory
* 5) YUMA_INSTALL/modules directory
*
* INPUTS:
*   helpmode == BRIEF, NORMAL or FULL 
*   logstdout == TRUE to use log_stdout
*                FALSE to use log_write
*
* RETURNS:
*   status of the operation
*********************************************************************/
status_t
    ncxmod_list_yang_files (help_mode_t helpmode,
                            boolean logstdout)
{
    xmlChar     *buff, *p;
    uint32       bufflen, pathlen;
    status_t    res;

    /* get a buffer to construct filespacs */
    bufflen = NCXMOD_MAX_FSPEC_LEN+1;
    buff = m__getMem(bufflen);
    if (!buff) {
        return ERR_INTERNAL_MEM;
    }

    /* 1) current directory */
    xml_strcpy(buff, (const xmlChar *)"./");
    res = list_subdirs(buff, 
                       bufflen,
                       SEARCH_TYPE_MODULE,
                       helpmode,
                       logstdout,
                       FALSE);
    if (res != NO_ERR) {
        m__free(buff);
        return res;
    }

    /* 2) try the NCX_MODPATH environment variable */
    if (ncxmod_mod_path) {
        res = list_pathlist(ncxmod_mod_path, 
                            buff,
                            bufflen,
                            SEARCH_TYPE_MODULE,
                            helpmode,
                            logstdout);
        if (res != NO_ERR) {
            m__free(buff);
            return res;
        }
    }

    /* 3) HOME/modules directory */
    if (ncxmod_home) {
        pathlen = xml_strlen(ncxmod_home);
        if (pathlen + 9 < bufflen) {
            p = buff;
            p += xml_strcpy(p, ncxmod_home);
            *p++ = NCXMOD_PSCHAR;
            p += xml_strcpy(p, (const xmlChar *)"modules");
            *p++ = NCXMOD_PSCHAR;
            *p = '\0';

            res = list_subdirs(buff, 
                               bufflen, 
                               SEARCH_TYPE_MODULE,
                               helpmode,
                               logstdout,
                               TRUE);
            if (res != NO_ERR) {
                m__free(buff);
                return res;
            }
        }
    }

    /* 4) YUMA_HOME/modules directory */
    if (ncxmod_yuma_home) {
        pathlen = xml_strlen(ncxmod_yuma_home);;
        if (pathlen + 9 < bufflen) {
            p = buff;
            p += xml_strcpy(p, ncxmod_yuma_home);
            *p++ = NCXMOD_PSCHAR;
            p += xml_strcpy(p, (const xmlChar *)"modules");
            *p++ = NCXMOD_PSCHAR;
            *p = '\0';

            res = list_subdirs(buff, 
                               bufflen, 
                               SEARCH_TYPE_MODULE,
                               helpmode,
                               logstdout,
                               TRUE);
            if (res != NO_ERR) {
                m__free(buff);
                return res;
            }
        }
    }

    /* 5) YUMA_INSTALL/modules directory */
    if (ncxmod_env_install) {
        pathlen = xml_strlen(ncxmod_env_install);;
        if (pathlen + 9 < bufflen) {
            p = buff;
            p += xml_strcpy(p, ncxmod_env_install);
            *p++ = NCXMOD_PSCHAR;
            p += xml_strcpy(p, (const xmlChar *)"modules");
            *p++ = NCXMOD_PSCHAR;
            *p = '\0';

            res = list_subdirs(buff, 
                               bufflen, 
                               SEARCH_TYPE_MODULE,
                               helpmode,
                               logstdout,
                               TRUE);
            if (res != NO_ERR) {
                m__free(buff);
                return res;
            }
        }
    }

    if (logstdout) {
        log_stdout("\n");
    } else {
        log_write("\n");
    }

    m__free(buff);
    return NO_ERR;

}  /* ncxmod_list_yang_files */


/********************************************************************
* FUNCTION ncxmod_setup_yumadir
*
* Setup the ~/.yuma directory if it does not exist
*
* RETURNS:
*   status
*********************************************************************/
status_t
    ncxmod_setup_yumadir (void)
{
    DIR           *dp;
    status_t       res;
    int            retcode;

    /* try to open yuma directory */
    res = NO_ERR;
    dp = opendir((const char *)ncxmod_yumadir_path);
    if (dp == NULL) {
        /* create a yuma directory with 700 permissions */
        retcode = mkdir((const char *)ncxmod_yumadir_path, S_IRWXU);
        if (retcode != 0) {
            res = errno_to_status();
        }
    } else {
        /* use the existing yuma directory */
        (void)closedir(dp);
    }

    if (res != NO_ERR) {
        log_error("\nError: Could not setup Yuma work directory\n");
    }

    return res;

}  /* ncxmod_setup_yumadir */


/********************************************************************
* FUNCTION ncxmod_setup_tempdir
*
* Setup the ~/.yuma/tmp directory if it does not exist
*
* RETURNS:
*   status
*********************************************************************/
status_t
    ncxmod_setup_tempdir (void)
{
    const xmlChar *tmpdir = NCXMOD_TEMP_DIR;
    xmlChar       *tempdir_path, *str;
    DIR           *dp;
    status_t       res;
    uint32         len;
    int            retcode;

    res = NO_ERR;

    /* make the string to use for the yuma temp directory */
    len = xml_strlen(ncxmod_yumadir_path) + xml_strlen(tmpdir) + 1;
    tempdir_path = m__getMem(len);
    if (tempdir_path == NULL) {
        return ERR_INTERNAL_MEM;
    }
    str = tempdir_path;
    str += xml_strcpy(str, ncxmod_yumadir_path);
    xml_strcpy(str, tmpdir);

    /* try to open yuma temp directory */
    dp = opendir((const char *)tempdir_path);
    if (dp == NULL) {
        /* create a yuma temp directory with 700 permissions */
        retcode = mkdir((const char *)tempdir_path, S_IRWXU);
        if (retcode != 0) {
            res = errno_to_status();
        }
    } else {
        /* use the existing ~/.yuma/tmp directory */
        (void)closedir(dp);
    }

    m__free(tempdir_path);

    return res;

}  /* ncxmod_setup_tempdir */


/********************************************************************
* FUNCTION ncxmod_new_program_tempdir
*
*   Setup a program instance temp files directory
*
* INPUTS:
*   res == address of return status
*
* OUTPUTS:
*  *res  == return status
*
* RETURNS:
*   malloced tempdir_progcb_t record (if NO_ERR)
*********************************************************************/
ncxmod_temp_progcb_t *
    ncxmod_new_program_tempdir (status_t *res)
{
    xmlChar              *buffer, *tempdir_path, *p;
    DIR                  *dp;
    ncxmod_temp_progcb_t *progcb = NULL;
    xmlChar               datebuff[TSTAMP_MIN_SIZE];
    xmlChar               numbuff[NCX_MAX_NUMLEN];
    int                   randnum, retcode;
    uint32                fixedlen, numlen;

    if (res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }

    /* setup */
    *res = NO_ERR;
    tstamp_datetime_dirname(datebuff);

    fixedlen = xml_strlen(ncxmod_yumadir_path) +
        xml_strlen(NCXMOD_TEMP_DIR);

    /* get positive 5 digit random number (0 -- 64k) */
    randnum = rand();
    if (randnum < 0) {
        randnum *= -1;
    }
    randnum &= 0xffff;
    snprintf((char *)numbuff, NCX_MAX_NUMLEN, "%d", randnum);
    numlen = xml_strlen(numbuff);

    /* get a buffer for the constructed directory name */
    buffer = m__getMem(TSTAMP_MIN_SIZE+fixedlen+numlen+2);
    if (buffer == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    /* construct the entire dir name string generate the default directory name
     * based on the current time + random number */
    p = buffer;
    p += xml_strcpy(p, ncxmod_yumadir_path);
    p += xml_strcpy(p, NCXMOD_TEMP_DIR);
    *p++ = NCXMOD_PSCHAR;
    p += xml_strcpy(p, datebuff);
    xml_strcpy(p, numbuff);

    /* covert this string to the expanded path name */
    tempdir_path = ncx_get_source(buffer, res);
    m__free(buffer);
    if (tempdir_path == NULL) {
        return NULL;
    }

    /* try to open ~/.yuma/tmp/<progstr> directory */
    dp = opendir((const char *)tempdir_path);
    if (dp == NULL) {
        /* create a directory with 700 permissions */
        retcode = mkdir((const char *)tempdir_path, S_IRWXU);
        if (retcode != 0) {
            *res = errno_to_status();
        }
    } else {
        /* error! this directory already exists, */
        *res = ERR_NCX_ENTRY_EXISTS;
        closedir( dp );
    }

    if (*res == NO_ERR) {
        progcb = new_temp_progcb();
        if (progcb == NULL) {
            *res = ERR_INTERNAL_MEM;
        } else {
            /* transfer malloced tempdir_path here */
            progcb->source = tempdir_path;
            tempdir_path = NULL;
        }
    }

    if (tempdir_path) {
        /* error exit */
        m__free(tempdir_path);
    }

    return progcb;

}  /* ncxmod_new_program_tempdir */


/********************************************************************
* FUNCTION ncxmod_free_program_tempdir
*
*   Remove a program instance temp files directory
*
* INPUTS:
*   progcb == tempoeray program instance control block to free
*
*********************************************************************/
void
    ncxmod_free_program_tempdir (ncxmod_temp_progcb_t *progcb)
{
#ifdef DEBUG
    if (progcb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    free_temp_progcb(progcb);

}  /* ncxmod_free_program_tempdir */


/********************************************************************
* FUNCTION ncxmod_new_session_tempdir
*
*   Setup a session instance temp files directory
*
* INPUTS:
*    progcb == program instance control block to use
*    sid == manager session ID of the new session instance control block
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*   malloced tempdir_sescb_t record
*********************************************************************/
ncxmod_temp_sescb_t *
    ncxmod_new_session_tempdir (ncxmod_temp_progcb_t *progcb,
                                uint32 sidnum,
                                status_t *res)
{
    xmlChar              *buffer, *p;
    DIR                  *dp;
    ncxmod_temp_sescb_t  *sescb;
    xmlChar               numbuff[NCX_MAX_NUMLEN];
    uint32                fixedlen, numlen;
    int                   retcode;


#ifdef DEBUG
    if (progcb == NULL || res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
    if (sidnum == 0 || progcb->source == NULL) {
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
#endif

    /* setup */
    *res = NO_ERR;
    sescb = NULL;
    fixedlen = xml_strlen(progcb->source);
    snprintf((char *)numbuff, NCX_MAX_NUMLEN, "%u", sidnum);
    numlen = xml_strlen(numbuff);

    /* get a buffer for the constructed directory name */
    buffer = m__getMem(fixedlen+numlen+2);
    if (buffer == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    /* construct the entire dir name string
     * generate the default directory name
     * based on the session number
     */
    p = buffer;
    p += xml_strcpy(p, progcb->source);
    *p++ = NCXMOD_PSCHAR;
    xml_strcpy(p, numbuff);

    /* already in expanded path name format
     * try to open ~/.yuma/tmp/<progstr>/<sidnum> directory */
    dp = opendir((const char *)buffer);
    if (dp == NULL) {
        /* create a directory with 700 permissions */
        retcode = mkdir((const char *)buffer, S_IRWXU);
        if (retcode != 0) {
            *res = errno_to_status();
        }
    } else {
        /* error! this directory already exists, */
        (void) closedir( dp );
        *res = ERR_NCX_ENTRY_EXISTS;
    }

    if (*res == NO_ERR) {
        sescb = new_temp_sescb();
        if (sescb == NULL) {
            *res = ERR_INTERNAL_MEM;
        } else {
            /* transfer malloced buffer here */
            sescb->source = buffer;         
            buffer = NULL;
            sescb->sidnum = sidnum;

            /* store pointer to the new session for cleanup */
            dlq_enque(sescb, &progcb->temp_sescbQ);
        }
    }

    if (buffer) {
        /* error exit */
        m__free(buffer);
    }

    return sescb;

}  /* ncxmod_new_session_tempdir */


/********************************************************************
* FUNCTION ncxmod_free_session_tempdir
*
*   Clean and free a session instance temp files directory
*
* INPUTS:
*    progcb == program instance control block to use
*    sidnum == manager session number to delete
*
*********************************************************************/
void
    ncxmod_free_session_tempdir (ncxmod_temp_progcb_t *progcb,
                                 uint32 sidnum)
{
    ncxmod_temp_sescb_t  *sescb;

#ifdef DEBUG
    if (progcb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
    if (sidnum == 0) {
        SET_ERROR(ERR_INTERNAL_VAL);
        return;
    }
#endif

    for (sescb = (ncxmod_temp_sescb_t *)
             dlq_firstEntry(&progcb->temp_sescbQ);
         sescb != NULL;
         sescb = (ncxmod_temp_sescb_t *)dlq_nextEntry(sescb)) {

        if (sescb->sidnum == sidnum) {
            dlq_remove(sescb);
            free_temp_sescb(sescb);
            return;
        }
    }

    /* sid not found */
    SET_ERROR(ERR_INTERNAL_VAL);    

}  /* ncxmod_free_session_tempdir */


/********************************************************************
* FUNCTION ncxmod_new_session_tempfile
*
*   Setup a session instance temp file for writing
*
* INPUTS:
*    sescb == session instance control block to use
*    filename == filename to create in the temp directory
*    res == address of return status
*
* OUTPUTS:
*    *res == return status
*
* RETURNS:
*   malloced tempdir_sescb_t record
*********************************************************************/
ncxmod_temp_filcb_t *
    ncxmod_new_session_tempfile (ncxmod_temp_sescb_t *sescb,
                                 const xmlChar *filename,
                                 status_t *res)
{
    xmlChar              *buffer, *p;
    ncxmod_temp_filcb_t  *filcb;
    uint32                fixedlen, filenamelen;

#ifdef DEBUG
    if (sescb == NULL || filename == NULL || res == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
    if (sescb->source == NULL) {
        *res = SET_ERROR(ERR_INTERNAL_VAL);
        return NULL;
    }
#endif

    /* first check if the filename is already used */
    for (filcb = (ncxmod_temp_filcb_t *)
             dlq_firstEntry(&sescb->temp_filcbQ);
         filcb != NULL;
         filcb = (ncxmod_temp_filcb_t *)dlq_nextEntry(filcb)) {

        if (!xml_strcmp(filcb->filename, filename)) {
            log_error("\nError: cannot create temp file '%s', "
                      "duplicate entry\n");
            *res = ERR_NCX_ENTRY_EXISTS;
            return NULL;
        }
    }

    /* setup */
    *res = NO_ERR;
    fixedlen = xml_strlen(sescb->source);
    filenamelen = xml_strlen(filename);

    /* get a buffer for the constructed file name */
    buffer = m__getMem(fixedlen+filenamelen+2);
    if (buffer == NULL) {
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }
    p = buffer;
    p += xml_strcpy(p, sescb->source);
    *p++ = NCXMOD_PSCHAR;
    xml_strcpy(p, filename);

    /* get a new file control block to return */
    filcb = new_temp_filcb();
    if (filcb == NULL) {
        m__free(buffer);
        *res = ERR_INTERNAL_MEM;
        return NULL;
    }

    /* transfer malloced buffer here */
    filcb->source = buffer;
    filcb->filename = p;

    /* store pointer to the new file for cleanup */
    dlq_enque(filcb, &sescb->temp_filcbQ);

    return filcb;

}  /* ncxmod_new_session_tempfile */


/********************************************************************
* FUNCTION ncxmod_free_session_tempfile
*
*   Clean and free a session instance temp files directory
*
* INPUTS:
*    filcb == file control block to delete
*
*********************************************************************/
void
    ncxmod_free_session_tempfile (ncxmod_temp_filcb_t *filcb)
{
#ifdef DEBUG
    if (filcb == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    /* supposeds to be part of the sescb->temp_filcbQ !!! */
    dlq_remove(filcb);
    free_temp_filcb(filcb);

}  /* ncxmod_free_session_tempfile */


/********************************************************************
* FUNCTION ncxmod_new_search_result
*
*  Malloc and initialize a search result struct
*
* RETURNS:
*   malloced and initialized struct, NULL if ERR_INTERNAL_MEM
*********************************************************************/
ncxmod_search_result_t *
    ncxmod_new_search_result (void)
{
    ncxmod_search_result_t *searchresult;

    searchresult = m__getObj(ncxmod_search_result_t);
    if (searchresult == NULL) {
        return NULL;
    }
    memset(searchresult, 0x0, sizeof(ncxmod_search_result_t));
    return searchresult;

}  /* ncxmod_new_search_result */


/********************************************************************
* FUNCTION ncxmod_new_search_result_ex
*
*  Malloc and initialize a search result struct
*  from a module source
*
* INPUTS:
*    mod == module struct to use
*
* RETURNS:
*   malloced and initialized struct, NULL if ERR_INTERNAL_MEM
*********************************************************************/
ncxmod_search_result_t *
    ncxmod_new_search_result_ex (const ncx_module_t *mod)
{
    ncxmod_search_result_t *searchresult;
    const xmlChar          *str;

#ifdef DEBUG
    if (mod == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    searchresult = m__getObj(ncxmod_search_result_t);
    if (searchresult == NULL) {
        return NULL;
    }
    memset(searchresult, 0x0, sizeof(ncxmod_search_result_t));

    searchresult->module = xml_strdup(mod->name);
    if (searchresult->module == NULL) {
        ncxmod_free_search_result(searchresult);
        return NULL;
    }

    if (mod->version) {
        searchresult->revision = xml_strdup(mod->version);
        if (searchresult->revision == NULL) {
            ncxmod_free_search_result(searchresult);
            return NULL;
        }
    }

    if (mod->ns) {
        searchresult->namespacestr = xml_strdup(mod->ns);
        if (searchresult->namespacestr == NULL) {
            ncxmod_free_search_result(searchresult);
            return NULL;
        }
        str = searchresult->namespacestr;
        while (*str && *str != '?') {
            str++;
        }
        searchresult->nslen = 
            (uint32)(str - searchresult->namespacestr);
    }

    if (mod->source) {
        searchresult->source = xml_strdup(mod->source);
        if (searchresult->source == NULL) {
            ncxmod_free_search_result(searchresult);
            return NULL;
        }
    }

    if (mod->belongs) {
        searchresult->belongsto = xml_strdup(mod->belongs);
        if (searchresult->belongsto == NULL) {
            ncxmod_free_search_result(searchresult);
            return NULL;
        }
    }

    searchresult->ismod = mod->ismod;
    
    return searchresult;

}  /* ncxmod_new_search_result_ex */


/********************************************************************
* FUNCTION ncxmod_new_search_result_str
*
*  Malloc and initialize a search result struct
*
* INPUTS:
*    modname == module name string to use
*    revision == revision date to use (may be NULL)
* RETURNS:
*   malloced and initialized struct, NULL if ERR_INTERNAL_MEM
*********************************************************************/
ncxmod_search_result_t *
    ncxmod_new_search_result_str (const xmlChar *modname,
                                  const xmlChar *revision)
{
    ncxmod_search_result_t *searchresult;

#ifdef DEBUG
    if (modname == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    searchresult = m__getObj(ncxmod_search_result_t);
    if (searchresult == NULL) {
        return NULL;
    }
    memset(searchresult, 0x0, sizeof(ncxmod_search_result_t));

    searchresult->module = xml_strdup(modname);
    if (searchresult->module == NULL) {
        ncxmod_free_search_result(searchresult);
        return NULL;
    }

    if (revision) {
        searchresult->revision = xml_strdup(revision);
        if (searchresult->revision == NULL) {
            ncxmod_free_search_result(searchresult);
            return NULL;
        }
    }
    searchresult->res = ERR_NCX_MOD_NOT_FOUND;
    return searchresult;

}  /* ncxmod_new_search_result_str */


/********************************************************************
* FUNCTION ncxmod_free_search_result
*
*  Clean and free a search result struct
*
* INPUTS:
*    searchresult == struct to clean and free
*********************************************************************/
void
    ncxmod_free_search_result (ncxmod_search_result_t *searchresult)
{
#ifdef DEBUG
    if (searchresult == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    if (searchresult->module) {
        m__free(searchresult->module);
    }
    if (searchresult->belongsto) {
        m__free(searchresult->belongsto);
    }
    if (searchresult->revision) {
        m__free(searchresult->revision);
    }
    if (searchresult->namespacestr) {
        m__free(searchresult->namespacestr);
    }
    if (searchresult->source) {
        m__free(searchresult->source);
    }
    m__free(searchresult);

}  /* ncxmod_free_search_result */


/********************************************************************
* FUNCTION ncxmod_clean_search_result_queue
*
*  Clean and free all the search result structs
*  in the specified Q
*
* INPUTS:
*    searchQ = Q of ncxmod_search_result_t to clean and free
*********************************************************************/
void
    ncxmod_clean_search_result_queue (dlq_hdr_t *searchQ)
{
    ncxmod_search_result_t *searchresult;

#ifdef DEBUG
    if (searchQ == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return;
    }
#endif

    while (!dlq_empty(searchQ)) {
        searchresult = (ncxmod_search_result_t *)dlq_deque(searchQ);
        ncxmod_free_search_result(searchresult);
    }

}  /* ncxmod_clean_search_result_queue */


/********************************************************************
* FUNCTION ncxmod_find_search_result
*
*  Find a search result in the specified Q
*
* Either modname or nsuri must be set
* If modname is set, then revision will be checked
*
* INPUTS:
*    searchQ = Q of ncxmod_search_result_t to check
*    modname == module or submodule name to find
*    revision == revision-date to find
*    nsuri == namespace URI fo find
* RETURNS:
*   pointer to first matching record; NULL if not found
*********************************************************************/
ncxmod_search_result_t *
    ncxmod_find_search_result (dlq_hdr_t *searchQ,
                               const xmlChar *modname,
                               const xmlChar *revision,
                               const xmlChar *nsuri)
{
    ncxmod_search_result_t *sr;
    const xmlChar          *str;
    uint32                 nslen;

#ifdef DEBUG
    if (searchQ == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    for (sr = (ncxmod_search_result_t *)dlq_firstEntry(searchQ);
         sr != NULL;
         sr = (ncxmod_search_result_t *)dlq_nextEntry(sr)) {

        if (modname) {
            if (sr->module == NULL ||
                xml_strcmp(sr->module, modname)) {
                continue;
            }
            if (revision) {
                if (sr->revision == NULL || 
                    xml_strcmp(sr->revision, revision)) {
                    continue;
                }
            }
            return sr;
        } else if (nsuri) {
            str = nsuri;
            while (*str && *str != '?') {
                str++;
            }
            nslen = (uint32)(str - nsuri);
            if (nslen == 0) {
                continue;
            }

            if (sr->namespacestr == NULL || sr->nslen == 0) {
                continue;
            }
            if (nslen != sr->nslen) {
                continue;
            }
            if (xml_strncmp(sr->namespacestr, nsuri, nslen)) {
                continue;
            }
            return sr;
        } else {
            SET_ERROR(ERR_INTERNAL_PTR);
            return NULL;
        }
    }
    return NULL;

}  /* ncxmod_find_search_result */


/********************************************************************
* FUNCTION ncxmod_clone_search_result
*
*  Clone a search result
*
* INPUTS:
*    sr = searchresult to clone
*
* RETURNS:
*   pointer to malloced and filled in clone of sr
*********************************************************************/
ncxmod_search_result_t *
    ncxmod_clone_search_result (const ncxmod_search_result_t *sr)
{
    ncxmod_search_result_t *newsr;

#ifdef DEBUG
    if (sr == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return NULL;
    }
#endif

    newsr = ncxmod_new_search_result();
    if (newsr == NULL) {
        return NULL;
    }

    if (sr->module) {
        newsr->module = xml_strdup(sr->module);
        if (newsr->module == NULL) {
            ncxmod_free_search_result(newsr);
            return NULL;
        }
    }

    if (sr->belongsto) {
        newsr->belongsto = xml_strdup(sr->belongsto);
        if (newsr->belongsto == NULL) {
            ncxmod_free_search_result(newsr);
            return NULL;
        }
    }

    if (sr->revision) {
        newsr->revision = xml_strdup(sr->revision);
        if (newsr->revision == NULL) {
            ncxmod_free_search_result(newsr);
            return NULL;
        }
    }

    if (sr->namespacestr) {
        newsr->namespacestr = xml_strdup(sr->namespacestr);
        if (newsr->namespacestr == NULL) {
            ncxmod_free_search_result(newsr);
            return NULL;
        }
    }

    if (sr->source) {
        newsr->source = xml_strdup(sr->source);
        if (newsr->source == NULL) {
            ncxmod_free_search_result(newsr);
            return NULL;
        }
    }

    newsr->mod = sr->mod;
    newsr->res = sr->res;
    newsr->nslen = sr->nslen;
    newsr->cap = sr->cap;
    newsr->capmatch = sr->capmatch;
    newsr->ismod = sr->ismod;

    return newsr;

}  /* ncxmod_clone_search_result */


/********************************************************************
* FUNCTION ncxmod_test_filespec
*
* Check the exact filespec to see if it a file
*
* INPUTS:
*    filespec == file spec to check
*
* RETURNS:
*    TRUE if valid readable file
*    FALSE otherwise
*********************************************************************/
boolean
    ncxmod_test_filespec (const xmlChar *filespec)
{
    int         ret;
    struct stat statbuf;

#ifdef DEBUG
    if (filespec == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return FALSE;
    }
#endif

    memset(&statbuf, 0x0, sizeof(statbuf));
    ret = stat((const char *)filespec, &statbuf);
    return (ret == 0 && S_ISREG(statbuf.st_mode)) ? TRUE : FALSE;

}  /* ncxmod_test_filespec */


/********************************************************************
* FUNCTION ncxmod_get_pathlen_from_filespec
*
* Get the length of the path part of the filespec string
*
* INPUTS:
*    filespec == file spec to check
*
* RETURNS:
*    number of chars to keep for the path spec
*********************************************************************/
uint32
    ncxmod_get_pathlen_from_filespec (const xmlChar *filespec)
{
    const xmlChar  *str;
    uint32          len;
    
#ifdef DEBUG
    if (filespec == NULL) {
        SET_ERROR(ERR_INTERNAL_PTR);
        return 0;
    }
#endif

    len = xml_strlen(filespec);
    if (len == 0) {
        return 0;
    }

    str = &filespec[len-1];

    while (*str && *str != NCXMOD_PSCHAR) {
        str--;
    }

    if (*str) {
        return (uint32)((str - filespec) + 1);
    } else {
        return 0;
    }
}  /* ncxmod_get_pathlen_from_filespec */


/* END file ncxmod.c */
